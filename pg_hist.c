#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "utils/array.h"
#include "utils/lsyscache.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define NMAXDIM 10
#define BATCH_SIZE 1000 

typedef struct {
	double mi;
	double ma;
	double delt;
	double mult;
} MyParam;

typedef struct 
{
	void *retarr;
	int *dims;	
	int nelem;
	int ndim;
	int lastpos; 
} MyInfo;



/* utilities that take Datum asa input and return bin location */
static int int16bin(Datum val, MyParam *par)
{
	int ret = (int)floor((DatumGetInt16(val) - par->mi) * par->mult);
	return ret;
}

static int int32bin(Datum val, MyParam *par)
{
	int ret = (int)floor((DatumGetInt32(val) - par->mi) * par->mult);
	return ret;
}

static int int64bin(Datum val, MyParam *par)
{
	int ret = (int)floor((DatumGetInt64(val) - par->mi) * par->mult);
	return ret;
}

static int float32bin(Datum val, MyParam *par)
{
	float4 ret = (DatumGetFloat4(val) - par->mi) * par->mult;
	int iret = -1;
	if (isfinite(ret))
	{
		iret = (int) floor(ret);
	}
	return iret;
}

static int float64bin(Datum val, MyParam *par)
{
	float8 ret = (DatumGetFloat8(val) - par->mi) * par->mult;
	int iret = -1;
	if (isfinite(ret))
	{
		iret = (int) floor(ret);
	}
	return iret;
}

static int numericbin(Datum val, MyParam *par)
{
	
	char *tmp;
	int iret = -1;
	double vald;
	Numeric num = DatumGetNumeric(val);
	if (!numeric_is_nan(num))
	{
		tmp = DatumGetCString(DirectFunctionCall1(numeric_out, val));
		vald  = strtod(tmp, NULL);
		pfree(tmp);
		iret = (int)floor((vald - par->mi) * par->mult);
	}
	return iret;
}

/* utilities that take Datum as input and convert it to double */

static double int16w(Datum val)
{
	return (double )DatumGetInt16(val);
}

static double int32w(Datum val)
{
	return (double) DatumGetInt32(val);
}

static double int64w(Datum val)
{
	return (double)DatumGetInt64(val);
}

static double float32w(Datum val)
{
	return (double) DatumGetFloat4(val);
}

static double float64w(Datum val)
{
	return (double) DatumGetFloat8(val);
}

static double numericw(Datum val)
{
	char *tmp;
	double vald;
	tmp = DatumGetCString(DirectFunctionCall1(numeric_out, val));
	vald  = strtod(tmp, NULL);
	pfree(tmp);
	return vald;
}

/* Main function that processes rows of Datums computes bin locations
 * and weight values 
 */
static int64_t processer(Datum *values, bool *isnull, TupleDesc td,
			int ndim, 
			double *mins, double *maxs,
			int *dims, int callid,  int *dimmults,
			MyParam *params, 
			int (*funcs[NMAXDIM]) (Datum, MyParam *),
			double (**funcW) (Datum),
			int weight_flag,
			double *weight
			)
{
	
	int i;
	int pos0 = 0;

	if (callid == 0)
	{

		for (i=1; i<=ndim; i++)
		{
			char *typ;
			if (i == 1)
			{
				dimmults[i-1] = 1;
			}
			else
			{
				dimmults[i-1] = dimmults[i-2] * dims[i-2];
			}
			typ = SPI_gettype(td, i); // type of column
			funcs[i - 1] = NULL;
			if (strcmp(typ, "int2") == 0)
			{
				funcs[i-1] = int16bin;
			}
			if (strcmp(typ, "int4") == 0) 
			{
				funcs[i-1] = int32bin;
			}
			if (strcmp(typ, "int8") == 0) 
			{
				funcs[i-1] = int64bin;
			}
			if (strcmp(typ, "float4") == 0)
			{
				funcs[i-1] = float32bin;
			}
			if (strcmp(typ, "float8") == 0)
			{
				funcs[i-1] = float64bin;
			}
			if (strcmp(typ, "numeric") == 0)
			{
				funcs[i-1] = numericbin;
			}
			pfree(typ);

			if (funcs[i-1] == NULL)
			{
				elog(ERROR, "Unrecognised input datatype");
			}
		
			params[i-1].mi = mins[i-1];
			params[i-1].ma = maxs[i-1];
			params[i-1].mult = dims[i-1] * 1. / (maxs[i-1] - mins[i-1]);
		}
		if (weight_flag)
		{
			char *typ = SPI_gettype(td, ndim+1); // type of column
			*funcW = NULL;
			if (strcmp(typ, "int2") == 0)
			{
				*funcW = int16w;
			}
			if (strcmp(typ, "int4") == 0) 
			{
				*funcW = int32w;
			}
			if (strcmp(typ, "int8") == 0) 
			{
				*funcW = int64w;
			}
			if (strcmp(typ, "float4") == 0)
			{
				*funcW = float32w;
			}
			if (strcmp(typ, "float8") == 0)
			{
				*funcW = float64w;
			}
			if (strcmp(typ, "numeric") == 0)
			{
				*funcW = numericw;
			}
			pfree(typ);

			if (*funcW == NULL)
			{
				elog(ERROR, "Unrecognised type of the weight column");
			}
		}
	}
	
	for (i=0; i<ndim; i++)
	{
		if (!isnull[i])
		{
			int posx = funcs[i](values[i], params + i);
			if ((posx == -1) || (posx >= dims[i]))
			{
				pos0 = -1;
				break;	
			}
			else
			{
				pos0 += dimmults[i] * posx;			
			}
		}
		else
		{
			pos0 = -1;
			break;
		}
	}
	if (weight_flag)
	{
		*weight = (*funcW)(values[ndim]);
	}
	return pos0;
}


Datum pg_hist_0(PG_FUNCTION_ARGS, int ndim, int weight_flag);

Datum pg_hist_0(PG_FUNCTION_ARGS, int ndim, int weight_flag)
{
	int proc, callid = 0;
	double mins[NMAXDIM];
	double maxs[NMAXDIM];
	int *dims;

	SPIPlanPtr curs;
	Portal port;
	Datum *elemsp=NULL;
	bool elmbyval, *nullsp=NULL;
	int pos, nelemsLen, nelemsMin, nelemsMax;
	int lastpos, returning;

	int nelem = 1;
	void* retarr;
	int64_t *retarr_i=0;
	float8 *retarr_d=0;
	Oid elmtype;
	int16_t elmlen;
	char elmalign = 0;
	ArrayType *minArr, *maxArr, *lenArr;
	FuncCallContext  *funcctx;
	MyInfo *info;

	if (SRF_IS_FIRSTCALL())
	{
		Datum *values = 0;
		bool *isnull = 0; // to prevent compiler complaints
		int dimmults[NMAXDIM];
		MyParam params[NMAXDIM];
		int (*funcs[NMAXDIM]) (Datum, MyParam *);
		double (*funcW) (Datum) = NULL;
		char *command;
		text *sql;
		int ncolumns;
		MemoryContext oldcontext; 
		
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		
		dims = (int *)palloc(NMAXDIM * sizeof(int));
		info = (MyInfo *)palloc(sizeof(MyInfo));

		info-> dims= dims;
		info->lastpos = -1; 
		sql = PG_GETARG_TEXT_P(0);
		/* Convert given text object to a C string */
		command = text_to_cstring(sql);

		lenArr = PG_GETARG_ARRAYTYPE_P(1);
		minArr = PG_GETARG_ARRAYTYPE_P(2);
		maxArr = PG_GETARG_ARRAYTYPE_P(3);
		
		if (	(ARR_NDIM(minArr) != 1) || 
			(ARR_NDIM(maxArr) != 1) || 
			(ARR_NDIM(lenArr) != 1))
		{
			elog(ERROR, "The arrays must be one-d" );
		}

		if (	array_contains_nulls(minArr) || 
			array_contains_nulls(maxArr) || 
			array_contains_nulls(lenArr))
		{
			elog(ERROR, "The arrays must not contain nulls" );	
		}

		elmtype = INT8OID;
		get_typlenbyvalalign(elmtype, &elmlen, &elmbyval, &elmalign);
		deconstruct_array(lenArr, elmtype, elmlen, elmbyval, elmalign,
					  &elemsp, &nullsp, &nelemsLen);
		if (nelemsLen > NMAXDIM)
		{
			elog(ERROR, "Too many elements");
		}
		for (int i = 0; i < nelemsLen; i++)
		{
			dims[i] = DatumGetInt64(elemsp[i]);
			if (dims[i] <= 0)
			{
				elog(ERROR,"The dimension must be > 0");
			}
		}
		elmtype = FLOAT8OID;
		get_typlenbyvalalign(elmtype, &elmlen, &elmbyval, &elmalign);
		deconstruct_array(minArr, elmtype, elmlen, elmbyval, elmalign,
					  &elemsp, &nullsp, &nelemsMin);
		if (nelemsMin > NMAXDIM)
		{
			elog(ERROR, "Too many elements");
		}

		for (int i=0; i<nelemsMin; i++)
		{
			mins[i] = DatumGetFloat8(elemsp[i]);
		}

		deconstruct_array(maxArr, elmtype, elmlen, elmbyval, elmalign,
					  &elemsp, &nullsp, &nelemsMax);
		if (nelemsMax > NMAXDIM)
		{
			elog(ERROR, "Too many elements");
		}
		for (int i=0; i<nelemsMax; i++)
		{
			maxs[i] = DatumGetFloat8(elemsp[i]);
		}

		if ((nelemsMin != nelemsMax) || (nelemsMin != nelemsLen))
		{
			elog(ERROR, "The length of all arrays must be the same");
		}
		if ((ndim!=0)&&(ndim!=nelemsMin))
		{
			elog(ERROR, "The dimensionality mismatch");	
		}
	
		ndim = nelemsMin;
		info->ndim = ndim;

		for(int i=0; i<ndim; i++)
		{
			nelem *= dims[i];
		}

		info->nelem = nelem; 
		if (nelem > (10*1000*1000))
		{
			elog(ERROR,"Array is to large");
		}

		if (weight_flag)
		{
			retarr = palloc(nelem * sizeof(float8));
			memset(retarr, 0, sizeof(float8) * nelem);
			retarr_d = (float8 *)retarr;		
			ncolumns = ndim + 1;

		}
		else
		{
			retarr = palloc(nelem * sizeof(int64_t));
			memset(retarr, 0, sizeof(int64_t) * nelem);
			retarr_i = (int64_t *)retarr;			
			ncolumns = ndim;
		}
		
		info->retarr = retarr;
		
		values = palloc(sizeof(Datum) * ncolumns);
		isnull = palloc(sizeof(bool) * ncolumns);

		SPI_connect();

		curs  = SPI_prepare_cursor(command, 0 , NULL, 0);
		
		if (curs == NULL)
		{
			elog(ERROR,"ERROR executing query");
		}

		port =  SPI_cursor_open("pg_hist_curs", curs,
			       NULL, NULL,
			       true);

		while (1)
		{
			SPI_scroll_cursor_fetch(port, FETCH_FORWARD,
				     BATCH_SIZE);
			proc = SPI_processed;
			if (proc == 0) { break;} 

			if ( SPI_tuptable != NULL)
			{
				TupleDesc tupdesc = SPI_tuptable->tupdesc;
				SPITupleTable *tuptable = SPI_tuptable;

				if (callid == 0) 
				{
					if (tupdesc->natts != ncolumns)
					{
						if (weight_flag)
						{

							elog(ERROR, "Number of columns must match the length of the arrays + plus an extrac column for the weights");
						}
						else
						{
							elog(ERROR, "Number of columns must match the length of the arrays");
						}
					}
				}
				
				for (int j = 0; j < proc; j++)
				{
					double cur_weight;
					HeapTuple tuple = tuptable->vals[j];
					heap_deform_tuple(tuple, tupdesc, values, isnull);
					pos = processer(values, isnull, tupdesc, ndim, mins, maxs, dims,
						callid, dimmults, params, funcs, &funcW, weight_flag,
						&cur_weight);
					callid++;
					if (pos >= 0)	
					{
						if (weight_flag)
						{
							retarr_d[pos] += cur_weight;
						}
						else
						{
							retarr_i[pos] += 1;
						}
					}
				}
			}
		}
		SPI_cursor_close(port);
		SPI_finish();
		pfree(command);
		pfree(values);
		pfree(isnull);
		funcctx->user_fctx = info;

        /* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc) != TYPEFUNC_COMPOSITE)
			ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			errmsg("function returning record called in context "
                            "that cannot accept type record")));
		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();
	info = (MyInfo*)(funcctx->user_fctx);
	retarr = info->retarr;
	if (weight_flag)
	{
		retarr_d = (float8 *)retarr;			
	}
	{
		retarr_i = (int64_t *)retarr;		
	}
	dims = info->dims;
	nelem = info->nelem;
	ndim = info->ndim;
	lastpos = info->lastpos + 1;
	returning = 0;
		
	for(;lastpos < nelem; lastpos++)
	{
		if (weight_flag)
		{
			if (retarr_d[lastpos] != 0)
			{
				returning = 1 ;
				break;
			}	
		}
		else
		{
			if (retarr_i[lastpos] != 0)
			{
				returning = 1 ;
				break;
			}	
		}
	}

	info->lastpos = lastpos;
	if (returning == 0)
	{
		pfree(retarr);
		SRF_RETURN_DONE(funcctx);
	}
	else
	{
		Datum result; 
		HeapTuple tuple; 
		bool isnullOut[NMAXDIM];
		Datum valuesOut[NMAXDIM];
		int pntr = lastpos; 
		for(int i=0;i<ndim;i++)
		{
			valuesOut[i] = Int32GetDatum(pntr % dims[i]); // i just copy by value
			pntr/=dims[i];
			isnullOut[i] = false;
		}
		if (weight_flag)
		{
			valuesOut[ndim] = Float8GetDatum(retarr_d[lastpos]); // copy by value again
		}
		else
		{
			valuesOut[ndim] = retarr_i[lastpos];
		}
		isnullOut[ndim] = false;

		tuple = heap_form_tuple(funcctx->tuple_desc,
				valuesOut,
				isnullOut);
		
		result = HeapTupleGetDatum(tuple);
		SRF_RETURN_NEXT(funcctx, result);
	}
}

PG_FUNCTION_INFO_V1(pg_hist);
Datum pg_hist(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo,0,0);
}

PG_FUNCTION_INFO_V1(pg_hist_1d);
Datum pg_hist_1d(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo,1,0);
}

PG_FUNCTION_INFO_V1(pg_hist_2d);
Datum pg_hist_2d(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo,2,0);
}

PG_FUNCTION_INFO_V1(pg_hist_3d);
Datum pg_hist_3d(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo,3,0);
}

PG_FUNCTION_INFO_V1(pg_hist_w);
Datum pg_hist_w(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo,0,1);
}

PG_FUNCTION_INFO_V1(pg_hist_1d_w);
Datum pg_hist_1d_w(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo,1,1);
}

PG_FUNCTION_INFO_V1(pg_hist_2d_w);
Datum pg_hist_2d_w(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo,2,1);
}

PG_FUNCTION_INFO_V1(pg_hist_3d_w);
Datum pg_hist_3d_w(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo,3,1);
}

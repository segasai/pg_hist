/*
	   Copyright (C) 2016-2017 Sergey Koposov

    Email: koposov@ast.cam.ac.uk

    This file is part of pg_hist.

    pg_hist is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    pg_hist is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pg_hist; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#if PG_VERSION_NUM >= 90300
#include "access/htup_details.h"
#else
#include "access/htup.h"
#endif
#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "utils/array.h"
#include "utils/lsyscache.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

// Parameters

// Maximum number of dimensions in the histogram
#define NMAXDIM 10

// Cursor batch size
#define BATCH_SIZE 1000 

// maximum number of elements in the histogram
#define MAXSIZE (100L*1000*1000)


typedef struct {
	double mi;
	double ma;
	double delt;
	double mult;
} MyParam;

typedef struct 
{
	void *histarr;
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
			bool weight_flag,
			double *weight
			)
{
	
	int i;
	int pos0 = 0;

	if (callid == 0)
	{

		for (i=1; i<=ndim; i++)
		{
			/* It is important that PG's indexing is starting
			 * from 1, while I use normal C indexing from 0
			 */
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
			char *typ = SPI_gettype(td, ndim + 1); // type of column
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
		/* if any of the columns is null we skip the row */
		if (!isnull[i])
		{
			int posx = funcs[i](values[i], params + i);
			if ((posx < 0) || (posx >= dims[i]))
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
		/* weight is the last column */
		*weight = (*funcW)(values[ndim]);
	}
	return pos0;
}


Datum pg_hist_0(PG_FUNCTION_ARGS, int ndim, bool weight_flag);

/* This is is the main histogram function 
 * on top of PG arguments which are 
 * query, nbins_array, left_bin_edge_array, right_bin_edge_array
 * this function accepts the ndim arguments for the number of dimensions of
 * the histogram. This ndim could be zero which means to guess the number of 
 * dimensions from array length. 
 * The last arguement is weight_flag which is either 1 or 0 to switch
 * between regimes with weighted histogram and normal histogram
 */
Datum pg_hist_0(PG_FUNCTION_ARGS, int ndim, bool weight_flag)
{
	int *dims;
	int lastpos;
	int nelem = 1;
	void* histarr;
	int64_t *histarr_i=0, retval_i=0;
	float8 *histarr_d=0, retval_d=0;
	bool last_row;
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
		Oid elmtype;
		int16_t elmlen;
		char elmalign = 0;
		Datum *elemsp=NULL;
		ArrayType *minArr, *maxArr, *lenArr;
		bool elmbyval, *nullsp=NULL;
		SPIPlanPtr prepared_plan;
		Portal portal;
		int nelemsLen, nelemsMin, nelemsMax;
		int i;
		double mins[NMAXDIM];
		double maxs[NMAXDIM];
		int callid = 0;
		
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		
		dims = (int *)palloc(NMAXDIM * sizeof(int));
		sql = PG_GETARG_TEXT_P(0);
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

		if (	(ARR_DIMS(minArr)[0] > NMAXDIM) || 
			(ARR_DIMS(maxArr)[0] > NMAXDIM) || 
			(ARR_DIMS(lenArr)[0] > NMAXDIM))
		{
			elog(ERROR, "Too many elements in the arrays");		
		}

		elmtype = INT8OID;
		get_typlenbyvalalign(elmtype, &elmlen, &elmbyval, &elmalign);
		deconstruct_array(lenArr, elmtype, elmlen, elmbyval, elmalign,
					  &elemsp, &nullsp, &nelemsLen);
		for (i = 0; i < nelemsLen; i++)
		{
			dims[i] = DatumGetInt64(elemsp[i]);
			if (dims[i] <= 0)
			{
				elog(ERROR,"All the histogram dimensions must be > 0");
			}
		}

		elmtype = FLOAT8OID;
		get_typlenbyvalalign(elmtype, &elmlen, &elmbyval, &elmalign);
		deconstruct_array(minArr, elmtype, elmlen, elmbyval, elmalign,
					  &elemsp, &nullsp, &nelemsMin);

		for (i=0; i<nelemsMin; i++)
		{
			mins[i] = DatumGetFloat8(elemsp[i]);
		}

		deconstruct_array(maxArr, elmtype, elmlen, elmbyval, elmalign,
					  &elemsp, &nullsp, &nelemsMax);
		for (i=0; i<nelemsMax; i++)
		{
			maxs[i] = DatumGetFloat8(elemsp[i]);
		}

		if ((nelemsMin != nelemsMax) || (nelemsMin != nelemsLen))
		{
			elog(ERROR, "The length of all arrays must be the same");
		}
		/* if ndim is zero then we guess ndim */
		if ((ndim != 0) && (ndim != nelemsMin))
		{
			elog(ERROR, "The dimensionality mismatch");	
		}
		ndim = nelemsMin;

		for (i=0; i<nelemsMax; i++)
		{
			if (maxs[i] <= mins[i])
			{
				elog(ERROR, "The values of upper bin edges should be strictly larger than lower bin edges");
			}
		}


		for(i=0; i<ndim; i++)
		{
			if (dims[i]>(INT_MAX/nelem))
			{
				/* overflow */
				elog(ERROR, "The histogram is too large (>%ld elements)", MAXSIZE);				
			}
			nelem *= dims[i];
		}

		if (nelem > MAXSIZE)
		{
			elog(ERROR,"The histogram is too large (>%ld elements)", MAXSIZE);
		}


		if (weight_flag)
		{
			histarr = palloc(nelem * sizeof(float8));
			memset(histarr, 0, sizeof(float8) * nelem);
			histarr_d = (float8 *)histarr;		
			ncolumns = ndim + 1;
		}
		else
		{
			histarr = palloc(nelem * sizeof(int64_t));
			memset(histarr, 0, sizeof(int64_t) * nelem);
			histarr_i = (int64_t *) histarr;			
			ncolumns = ndim;
		}

		info = (MyInfo *) palloc(sizeof(MyInfo));
		info->dims = dims;
		info->lastpos = -1; 
		info->nelem = nelem; 
		info->ndim = ndim;		
		info->histarr = histarr;
		
		values = palloc(sizeof(Datum) * ncolumns);
		isnull = palloc(sizeof(bool) * ncolumns);

		SPI_connect();

		prepared_plan = SPI_prepare_cursor(command, 0 , NULL, 
				CURSOR_OPT_NO_SCROLL);
		
		if (prepared_plan == NULL)
		{
			elog(ERROR,"ERROR executing query");
		}

		portal = SPI_cursor_open("pg_hist_curs", prepared_plan,
			       NULL, NULL, true);

		while(1)
		{
			int nrows; 
			TupleDesc tupdesc;
			SPITupleTable *tuptable;
			
			SPI_scroll_cursor_fetch(portal, FETCH_FORWARD, BATCH_SIZE);
			nrows = SPI_processed;

			if ((nrows == 0) || ( SPI_tuptable == NULL)) 
			{
				break;
			} 

			tupdesc = SPI_tuptable->tupdesc;
			tuptable = SPI_tuptable;
			
			/* In ther first iteration I check whether 
			 * the number of columns is correct
			 */
			if (callid == 0) 
			{
				if (tupdesc->natts != ncolumns)
				{
					if (weight_flag)
					{

						elog(ERROR, "Number of columns must match the length of the arrays + plus an extra column for the weights");
					}
					else
					{
						elog(ERROR, "Number of columns must match the length of the arrays");
					}
				}
			}
			
			for (i = 0; i < nrows; i++)
			{
				double cur_weight;
				int histpos;
				HeapTuple tuple = tuptable->vals[i];
				heap_deform_tuple(tuple, tupdesc, values, isnull);
				histpos = processer(values, isnull, tupdesc, ndim, mins, maxs, dims,
					callid, dimmults, params, funcs, &funcW, weight_flag,
					&cur_weight);
				callid++;
				if (histpos >= 0)	
				{
					if (weight_flag)
					{
						histarr_d[histpos] += cur_weight;
					}
					else
					{
						histarr_i[histpos] += 1;
					}
				}
			}
			SPI_freetuptable(SPI_tuptable);
		}
		
		SPI_cursor_close(portal);
		SPI_freeplan(prepared_plan);
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
	info = (MyInfo*) (funcctx->user_fctx);
	histarr = info->histarr;
	if (weight_flag)
	{
		histarr_d = (float8 *) histarr;			
	}
	{
		histarr_i = (int64_t *) histarr;		
	}
	dims = info->dims;
	nelem = info->nelem;
	ndim = info->ndim;
	lastpos = info->lastpos + 1;
	last_row = true;
		
	for(;lastpos < nelem; lastpos++)
	{
		if (weight_flag)
		{
			retval_d = histarr_d[lastpos];
			if (retval_d != 0)
			{
				last_row = false ;
				break;
			}	
		}
		else
		{
			retval_i = histarr_i[lastpos];
			if (retval_i != 0)
			{
				last_row = false ;
				break;
			}	
		}
	}

	info->lastpos = lastpos;
	if (last_row)
	{
		pfree(histarr);
		pfree(dims);
		pfree(info);
		SRF_RETURN_DONE(funcctx);
	}
	else
	{
		Datum result; 
		HeapTuple tuple; 
		bool isnullOut[NMAXDIM + 1];
		Datum valuesOut[NMAXDIM + 1];
		int pntr = lastpos; 
		int i;
		for(i=0; i<ndim; i++)
		{
			valuesOut[i] = Int32GetDatum(pntr % dims[i]); // i just copy by value
			pntr /= dims[i];
			isnullOut[i] = false;
		}
		if (weight_flag)
		{
			valuesOut[ndim] = Float8GetDatum(retval_d); // copy by value again
		}
		else
		{
			valuesOut[ndim] = retval_i;
		}
		isnullOut[ndim] = false;

		tuple = heap_form_tuple(funcctx->tuple_desc,
				valuesOut,
				isnullOut);
		
		result = HeapTupleGetDatum(tuple);
		SRF_RETURN_NEXT(funcctx, result);
	}
}

/* These are the wrappers around pg_hist_0 */

PG_FUNCTION_INFO_V1(pg_hist);
Datum pg_hist(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo, 0, false);
}

PG_FUNCTION_INFO_V1(pg_hist_1d);
Datum pg_hist_1d(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo, 1, false);
}

PG_FUNCTION_INFO_V1(pg_hist_2d);
Datum pg_hist_2d(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo, 2, false);
}

PG_FUNCTION_INFO_V1(pg_hist_3d);
Datum pg_hist_3d(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo, 3, false);
}

PG_FUNCTION_INFO_V1(pg_hist_w);
Datum pg_hist_w(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo, 0, true);
}

PG_FUNCTION_INFO_V1(pg_hist_1d_w);
Datum pg_hist_1d_w(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo, 1, true);
}

PG_FUNCTION_INFO_V1(pg_hist_2d_w);
Datum pg_hist_2d_w(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo, 2, true);
}

PG_FUNCTION_INFO_V1(pg_hist_3d_w);
Datum pg_hist_3d_w(PG_FUNCTION_ARGS)
{
	return pg_hist_0(fcinfo, 3, true);
}

#include "postgres.h"

#include "executor/spi.h"
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "utils/array.h"
#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "utils/lsyscache.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

//int pg_hist(text *sql, int cnt);
#define NMAXDIM 10

/*
mi ma nbins
(int) (x-mi)/(ma-mi)*(nbins)

*/
typedef struct {
double mi;
double ma;
double delt;
double mult;
} MyParam;


static int int16bin(Datum val, MyParam *par)
{
	int ret = (int)((DatumGetInt16(val)-par->mi)*par->mult);
	return ret;
}

static int int32bin(Datum val, MyParam *par)
{
	int ret = (int)((DatumGetInt32(val)-par->mi)*par->mult);
	return ret;
}

static int int64bin(Datum val, MyParam *par)
{
	int ret = (int)((DatumGetInt64(val)-par->mi)*par->mult);
	return ret;
}

static int float32bin(Datum val, MyParam *par)
{
	float4 ret = (DatumGetFloat4(val)-par->mi)*par->mult;
	int iret=-1;
	if (!isfinite(ret))
	{
		iret = (int) ret;
	}
	return iret;
}

static int float64bin(Datum val, MyParam *par)
{
	float8 ret = (DatumGetFloat8(val)-par->mi)*par->mult;
	int iret=-1;
	if (!isfinite(ret))
	{
		iret = (int) ret;
	}
	return iret;
}

static int numericbin(Datum val, MyParam *par)
{
	
	char *tmp;
	int iret=-1;
	double vald;
	Numeric num = DatumGetNumeric(val);
	if (!numeric_is_nan(num))
	{
		tmp = DatumGetCString(DirectFunctionCall1(numeric_out,val));
		vald  = strtod(tmp, NULL);
		pfree(tmp);
		iret = (int)((vald-val-par->mi)*par->mult);
	}
	return iret;
}


static int64_t func(Datum *values, bool *isnull, TupleDesc td,
			double *mins, double *maxs,
			int *dims, int callid)
{
	
	static int ncols;
	int i;
	static int (*funcs[NMAXDIM]) (Datum, MyParam *);
	static MyParam params[NMAXDIM];
	static int dimmults[NMAXDIM];
	if (callid==0)
	{
		ncols = td->natts;
		for (i=1;i<=ncols;i++)
		{
			if (i==1)
			{
				dimmults[i-1] = 1;
			}
			else
			{
				dimmults[i-1] = dimmults[i-2] * dims[i-2];
			}
			char *typ = SPI_gettype(td, i); // type of column
			funcs[i-1]=NULL;
			if (strcmp(typ,"int2")==0) {funcs[i-1] = int16bin;}
			if (strcmp(typ,"int4")==0) {funcs[i-1] = int32bin;}
			if (strcmp(typ,"int8")==0) {funcs[i-1] = int64bin;}
			if (strcmp(typ,"float4")==0) {funcs[i-1] = float32bin;}
			if (strcmp(typ,"float8")==0) {funcs[i-1] = float64bin;}
			if (strcmp(typ,"numeric")==0) {funcs[i-1] = numericbin;}
			pfree(typ);

			if (funcs[i-1]==NULL)
			{
				elog(ERROR,"Wrong type");
			}
		
			params[i-1].mi=mins[i-1];
			params[i-1].ma=maxs[i-1];
			params[i-1].mult=dims[i-1]*1./(maxs[i-1]-mins[i-1]);
		}		
	}
	int pos0 = -1;
	for (i=0; i<ncols;i++)
	{
		if (!isnull[i])
		{
			int posx = funcs[i](values[i],params+i);
			if (posx!=-1)
			{
				pos0 += dimmults[i]*posx;
			}
		}
	}
	
	return pos0;
}

PG_FUNCTION_INFO_V1(pg_hist);
//int pg_hist(text *sql, int cnt)
Datum pg_hist(PG_FUNCTION_ARGS)
{
	char *command;
	int proc;
	int ndim = 2; 
	double mins[NMAXDIM];
	double maxs[NMAXDIM];
	int dims[NMAXDIM];
	SPIPlanPtr curs;
	Portal port;
	int nbatch = 100;
	int callid = 0;
	Datum *values;
	bool *isnull;
	int pos; 
	int nelem=1;
	Oid elmtype;
	int16_t elmlen;
	bool elmbyval;
	char elmalign=0;
	ArrayType *minArr, *maxArr, *lenArr; 
	Datum *elemsp=NULL;
	bool *nullsp=NULL;
	int nelemsLen, nelemsMin, nelemsMax;
	text *sql = PG_GETARG_TEXT_P(0);
	lenArr = PG_GETARG_ARRAYTYPE_P(1);
	minArr = PG_GETARG_ARRAYTYPE_P(2);
	maxArr = PG_GETARG_ARRAYTYPE_P(3);

	if ((ARR_NDIM(minArr)!=1)||(ARR_NDIM(maxArr)!=1)||(ARR_NDIM(lenArr)!=1))
	{
		elog(ERROR, "The arrays must be one-d" );
	}

	if (array_contains_nulls(minArr)||array_contains_nulls(maxArr)||array_contains_nulls(lenArr))
	{
		elog(ERROR, "The array of mins mustn't contain nulls" );	
	}

	elmtype= INT8OID;
	get_typlenbyvalalign(elmtype, &elmlen, &elmbyval, &elmalign);
	deconstruct_array(minArr, elmtype, elmlen, elmbyval, elmalign,
                                  &elemsp, &nullsp, &nelemsLen);
	if (nelemsLen>NMAXDIM)
	{
		elog(ERROR, "Too many elements");
	}
	for (int i=0;i<nelemsLen;i++)
	{
		dims[i] = DatumGetInt64(elemsp[i]);
	}


	elmtype= FLOAT8OID;
	get_typlenbyvalalign(elmtype, &elmlen, &elmbyval, &elmalign);
	deconstruct_array(minArr, elmtype, elmlen, elmbyval, elmalign,
                                  &elemsp, &nullsp, &nelemsMin);
	if (nelemsMin>NMAXDIM)
	{
		elog(ERROR, "Too many elements");
	}

	for (int i=0;i<nelemsMin;i++)
	{
		mins[i] = DatumGetFloat8(elemsp[i]);
	}

	deconstruct_array(maxArr, elmtype, elmlen, elmbyval, elmalign,
                                  &elemsp, &nullsp, &nelemsMax);
	if (nelemsMax>NMAXDIM)
	{
		elog(ERROR, "Too many elements");
	}
	for (int i=0;i<nelemsMax;i++)
	{
		maxs[i] = DatumGetFloat8(elemsp[i]);
	}

	if ((nelemsMin!=nelemsMax)||(nelemsMin!=nelemsLen))
	{
		elog(ERROR, "The length of all arrays must be the same");
	}
	ndim = nelemsMin;
	
	for(int i=0;i<ndim;i++)
	{
		nelem*=dims[i];
	}
	if (nelem>(10*1000*1000))
	{
		elog(ERROR,"Array is to large");
	}

	long *retarr = palloc(nelem*sizeof(long));

	/* Convert given text object to a C string */
	command = text_to_cstring(sql);

	SPI_connect();

	//ret = SPI_exec(command, 0);
	curs  = SPI_prepare_cursor(command, 0 , NULL, 0);
	if (curs == NULL)
	{
		elog(ERROR,"ERROR");
	}
	port =  SPI_cursor_open("pg_hist_curs", curs,
                       NULL, NULL,
                       true);
	SPI_scroll_cursor_fetch(port, FETCH_FORWARD,
                             nbatch);
	proc = SPI_processed;
	/*
	 * If some rows were fetched, print them via elog(INFO).
	 */
	if ( SPI_tuptable != NULL)
	{
		elog(INFO, "HERE %d",proc);

		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		char *typ1= SPI_gettype(tupdesc, 1); // type of column
		elog(INFO, "HERE %s", typ1);

		SPITupleTable *tuptable = SPI_tuptable;
		//char buf[8192];
		int j;
		if (tupdesc->natts != ndim)
		{
			elog(ERROR, "Mismatch in number of columns");
		}
		values = palloc(sizeof(Datum) * tupdesc->natts);
		isnull = palloc(sizeof(bool) * tupdesc->natts);

		for (j = 0; j < proc; j++)
		{
			HeapTuple tuple = tuptable->vals[j];
			heap_deform_tuple(tuple, tupdesc, values, isnull);

			pos = func(values, isnull, tupdesc, mins, maxs, dims,
					callid);
			callid++;	
			retarr[pos] += 1;
			//for (i = 1, buf[0] = 0; i <= tupdesc->natts; i++)
			//	snprintf(buf + strlen (buf), sizeof(buf) - strlen(buf), " %s%s",
			//		SPI_getvalue(tuple, tupdesc, i),
			//		(i == tupdesc->natts) ? " " : " |");
			//elog(INFO, "EXECQ: %s", buf);
		}
	}
	SPI_cursor_close(port);
	SPI_finish();
	pfree(command);

	return (proc);
}

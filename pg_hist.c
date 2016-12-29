#include "postgres.h"

#include "executor/spi.h"
#include "utils/builtins.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

int pg_hist(text *sql, int cnt);
#define NMAXDIM 10
int
pg_hist(text *sql, int cnt)
{
	char *command;
	int ret;
	int proc;
	int ndim = 2; 
	double mins[NMAXDIM];
	double maxs[NMAXDIM];
	int dims[NMAXDIM];
	mins[0] = 0;
	maxs[0]= 5;	
	mins[1] = -1;
	maxs[1]= 10;
	dims[0] =10;
	dims[1]= 30;
	

	/* Convert given text object to a C string */
	command = text_to_cstring(sql);

	SPI_connect();

	ret = SPI_exec(command, 0);

	proc = SPI_processed;
	/*
	 * If some rows were fetched, print them via elog(INFO).
	 */
	if (ret > 0 && SPI_tuptable != NULL)
	{
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		SPITupleTable *tuptable = SPI_tuptable;
		char buf[8192];
		int i, j;

		for (j = 0; j < proc; j++)
		{
			HeapTuple tuple = tuptable->vals[j];

			for (i = 1, buf[0] = 0; i <= tupdesc->natts; i++)
				snprintf(buf + strlen (buf), sizeof(buf) - strlen(buf), " %s%s",
					SPI_getvalue(tuple, tupdesc, i),
					(i == tupdesc->natts) ? " " : " |");
			elog(INFO, "EXECQ: %s", buf);
		}
	}

	SPI_finish();
	pfree(command);

	return (proc);
}

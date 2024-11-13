#include <iostream>
#include <functional>//for std::functions
#include <cstring>
#include <cstdlib>

#include <windows.h>
#include <fileapi.h>

#include "Table.h"
#include "Query.h"
#include "Parameters.h"

//check memory leaks
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


void test1(Table& table);
void test2(Table& table);

int main(int argc, char* argv[]) {
	char name1[] = "c1";
	char name2[] = "c2";
	char type[] = "int";
	Table table = Table((char*)"tableT", 0, 0);
	table.AddColumn(name1, type, SHORT_INT_SIZE);
	table.AddColumn(name2, type, SHORT_INT_SIZE);
	table.ConfirmDiagram();
	table.DisplayColumns();

	test1(table);

	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	//_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	//_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	//_CrtDumpMemoryLeaks();

	return 0;
}

void test1(Table& table) {
	for (int i = 0; i < 2 * 128; i++) {
		int* a1 = (int*)malloc(4);
		int* a2 = (int*)malloc(4);
		*a1 = i;
		*a2 = (i * 200);
		void* argss[] = { a1, a2 };
		std::cout << "adding " << *(int*)a1 << " " << *(int*)a2 << std::endl;
		table.AddRow(argss);
		free(a1);
		free(a2);
		//free(argss);
	}

	uint32_t* a11 = (uint32_t*)malloc(sizeof(uint32_t));
	uint32_t* a22 = (uint32_t*)malloc(sizeof(uint32_t));
	uint32_t* a33 = (uint32_t*)malloc(sizeof(uint32_t));
	*a11 = 180;
	*a22 = 89;
	*a33 = 100;
	void* argss[] = { a11 };

	uint32_t* a111 = (uint32_t*)malloc(sizeof(uint32_t));
	uint32_t* a222 = (uint32_t*)malloc(sizeof(uint32_t));
	uint32_t* a333 = (uint32_t*)malloc(sizeof(uint32_t));
	*a111 = 1 * 200;
	*a222 = 89 * 200;
	*a333 = 100 * 200;
	void* argsss[] = { a111 };

	Query* query = new Query();
	query->FindByComparator(&table, (char*)"c1", argss, 1, EQUALS | BIGGER);
	query->FindByComparator(&table, (char*)"c2", argsss, 1, EQUALS);
	query->CompareQueries(AND);
}

void test2(Table& table) {
	int* a1 = (int*)malloc(4);
	int* a2 = (int*)malloc(4);
	*a1 = 10;
	*a2 = 5;
	void* argss[] = { a1, a2 };
	table.AddRow(argss);

	int* a11 = (int*)malloc(4);
	int* a21 = (int*)malloc(4);
	*a11 = 5;
	*a21 = 6;
	void* argss0[] = { a11, a21 };
	table.AddRow(argss0);

	int* a12 = (int*)malloc(4);
	int* a22 = (int*)malloc(4);
	*a12 = 8;
	*a22 = 7;
	void* argss1[] = { a12, a22 };
	table.AddRow(argss1);

	int* a172 = (int*)malloc(4);
	int* a272 = (int*)malloc(4);
	*a172 = 7;
	*a272 = 1;
	void* argss17[] = { a172, a272 };
	table.AddRow(argss17);

	int* a128 = (int*)malloc(4);
	int* a228 = (int*)malloc(4);
	*a128 = 2;
	*a228 = 7;
	void* argss18[] = { a128, a228 };
	table.AddRow(argss18);


	int* a15 = (int*)malloc(4);
	*a15 = 20;
	void* argssq[] = { a15};
	Query* query = new Query();
	query->FindByComparator(&table, (char*)"c1", argssq, 1, EQUALS | LESS);

}
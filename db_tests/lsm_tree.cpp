#include <iostream>
#include <functional>//for std::functions
#include <cstring>
#include <cstdlib>

#include <windows.h>
#include <fileapi.h>

#include "Table.h"
#include "Conditions.h"
#include "Parameters.h"




int main(int argc, char* argv[]) {
	char name1[] = "c1";
	char name2[] = "c2";
	char type[] = "int";
	Table table = Table((char*)"tableT", 0, 0);
	table.AddColumn(name1, type, SHORT_INT_SIZE);
	table.AddColumn(name2, type, SHORT_INT_SIZE);
	table.ConfirmDiagram();
	table.DisplayColumns();
	
	//uint32_t* data1 = (uint32_t*)malloc(sizeof(uint32_t));
	//*data1 = 1;
	//uint32_t* data2 = (uint32_t*)malloc(sizeof(uint32_t));
	//*data2 = 10;
	//void* newData[] = { data1, data2 };

	//uint32_t* data11 = (uint32_t*)malloc(sizeof(uint32_t));
	//*data11 = 2;
	//uint32_t* data22 = (uint32_t*)malloc(sizeof(uint32_t));
	//*data22 = 11;
	//void* newData2[] = { data11, data22 };

	//uint32_t* data111 = (uint32_t*)malloc(sizeof(uint32_t));
	//*data111 = 3;
	//uint32_t* data222 = (uint32_t*)malloc(sizeof(uint32_t));
	//*data222 = 12;
	//void* newData3[] = { data111, data222 };

	//table.AddRow(newData);
	//table.AddRow(newData2);
	//table.AddRow(newData3);

	//table.DisplayAllRows();

	//uint32_t* a1 = (uint32_t*)malloc(sizeof(uint32_t));
	//*a1 = 2;
	//uint32_t* a2 = (uint32_t*)malloc(sizeof(uint32_t));
	//*a2 = 1;
	//uint32_t* a3 = (uint32_t*)malloc(sizeof(uint32_t));
	//*a3 = 9;
	//void* arg[] = { a1, a2, a3 };

	//Conditions::equals(&table, (char*)"c1", arg, 3);

	for (int i = 0; i < 2 * 128; i++) {
		int* a1 = (int*)malloc(4);
	    int* a2 = (int*)malloc(4);
		*a1 = i;
		*a2 = (i * 200);
		void* argss[] = { a1, a2 };
		std::cout << "adding " << *(int*)a1 << " " << *(int*)a2 << std::endl;
		table.AddRow(argss);
	}

	uint32_t* a11 = (uint32_t*)malloc(sizeof(uint32_t));
	uint32_t* a22 = (uint32_t*)malloc(sizeof(uint32_t));
	*a11 = 1;
	*a22 = 0;
	void* argss[] = { a11, a22 };
	//Conditions::equals(&table, (char*)"c1", argss, 2);

}
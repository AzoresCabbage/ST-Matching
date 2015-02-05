#include "stdafx.h"
#include "database.h"

bool Database::connDB(){
	conn = PQconnectdb(connInfo.c_str());
	if(PQstatus(conn) == CONNECTION_BAD){
		std::cerr<<"Database connection failed!"<<std::endl;
		return false;
	}
	else{
		std::cerr<<"Database connection success!"<<std::endl;
		return true;
	}
}

void Database::closeConn(){
	PQfinish(conn);
}

PGresult* Database::execQuery(std::string SQL){

	PGresult* res = PQexec(conn,SQL.c_str());
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
		std::cerr<<"SQL query error! statement:"<<SQL<<std::endl;
		PQclear(res);
		//closeConn(conn);
		return NULL;
	}
	return res;
}

bool Database::execUpdate(std::string SQL){
	PGresult* res = PQexec(conn,SQL.c_str());
	if(PQresultStatus(res) != PGRES_COMMAND_OK){
		//std::ofstream fout("output.txt");
		std::cerr<<"SQL update error! statement:"<<SQL<<std::endl;
		//fout<<SQL<<std::endl;
		//fout.close();
		PQclear(res);
		//closeConn(conn);
		return false;
	}
	return true;
}
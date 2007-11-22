//
// ODBCSQLServerTest.cpp
//
// $Id: //poco/Main/Data/ODBC/testsuite/src/ODBCSQLServerTest.cpp#5 $
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "ODBCSQLServerTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/String.h"
#include "Poco/Format.h"
#include "Poco/Any.h"
#include "Poco/DynamicAny.h"
#include "Poco/Tuple.h"
#include "Poco/DateTime.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/ODBC/Diagnostics.h"
#include "Poco/Data/ODBC/ODBCException.h"
#include "Poco/Data/ODBC/ODBCStatementImpl.h"
#include <iostream>


using namespace Poco::Data;
using ODBC::Utility;
using ODBC::ConnectionException;
using ODBC::StatementException;
using ODBC::StatementDiagnostics;
using Poco::format;
using Poco::Tuple;
using Poco::Any;
using Poco::AnyCast;
using Poco::DynamicAny;
using Poco::DateTime;


#ifdef POCO_OS_FAMILY_WINDOWS
	#ifdef POCO_ODBC_USE_SQL_NATIVE
		#define MS_SQL_SERVER_ODBC_DRIVER "SQL Native Client"
	#else
		#define MS_SQL_SERVER_ODBC_DRIVER "SQL Server"
	#endif
#else
	#define MS_SQL_SERVER_ODBC_DRIVER "FreeTDS"
#endif
#define MS_SQL_SERVER_DSN "PocoDataSQLServerTest"
#define MS_SQL_SERVER_SERVER "localhost"
#define MS_SQL_SERVER_PORT "1433"
#define MS_SQL_SERVER_DB "test"
#define MS_SQL_SERVER_UID "test"
#define MS_SQL_SERVER_PWD "test"


ODBCTest::SessionPtr ODBCSQLServerTest::_pSession;
ODBCTest::ExecPtr    ODBCSQLServerTest::_pExecutor;
std::string          ODBCSQLServerTest::_driver = MS_SQL_SERVER_ODBC_DRIVER;
std::string          ODBCSQLServerTest::_dsn = MS_SQL_SERVER_DSN;
std::string          ODBCSQLServerTest::_uid = MS_SQL_SERVER_UID;
std::string          ODBCSQLServerTest::_pwd = MS_SQL_SERVER_PWD;
std::string          ODBCSQLServerTest::_db  = MS_SQL_SERVER_DB;
std::string ODBCSQLServerTest::_connectString = "DRIVER=" MS_SQL_SERVER_ODBC_DRIVER ";"
	"UID=" MS_SQL_SERVER_UID ";"
	"PWD=" MS_SQL_SERVER_PWD ";"
	"DATABASE=" MS_SQL_SERVER_DB ";"
	"SERVER=" MS_SQL_SERVER_SERVER ";"
	"PORT=" MS_SQL_SERVER_PORT ";";


ODBCSQLServerTest::ODBCSQLServerTest(const std::string& name): 
	ODBCTest(name, _pSession, _pExecutor, _dsn, _uid, _pwd, _connectString)
{
}


ODBCSQLServerTest::~ODBCSQLServerTest()
{
}


void ODBCSQLServerTest::testBareboneODBC()
{
	std::string tableCreateString = "CREATE TABLE Test "
		"(First VARCHAR(30),"
		"Second VARCHAR(30),"
		"Third VARBINARY(30),"
		"Fourth INTEGER,"
		"Fifth FLOAT,"
		"Sixth DATETIME)";

	//TODO: auto binding fails at SQLExecute() ("String data, right truncated")
	//executor().bareboneODBCTest(dbConnString(), tableCreateString, SQLExecutor::PB_IMMEDIATE, SQLExecutor::DE_MANUAL);
	//executor().bareboneODBCTest(dbConnString(), tableCreateString, SQLExecutor::PB_IMMEDIATE, SQLExecutor::DE_BOUND);
	executor().bareboneODBCTest(dbConnString(), tableCreateString, SQLExecutor::PB_AT_EXEC, SQLExecutor::DE_MANUAL);
	executor().bareboneODBCTest(dbConnString(), tableCreateString, SQLExecutor::PB_AT_EXEC, SQLExecutor::DE_BOUND);

	tableCreateString = "CREATE TABLE Test "
		"(First VARCHAR(30),"
		"Second INTEGER,"
		"Third FLOAT)";

	executor().bareboneODBCMultiResultTest(dbConnString(), tableCreateString, SQLExecutor::PB_IMMEDIATE, SQLExecutor::DE_MANUAL);
	executor().bareboneODBCMultiResultTest(dbConnString(), tableCreateString, SQLExecutor::PB_IMMEDIATE, SQLExecutor::DE_BOUND);
	executor().bareboneODBCMultiResultTest(dbConnString(), tableCreateString, SQLExecutor::PB_AT_EXEC, SQLExecutor::DE_MANUAL);
	executor().bareboneODBCMultiResultTest(dbConnString(), tableCreateString, SQLExecutor::PB_AT_EXEC, SQLExecutor::DE_BOUND);
}


void ODBCSQLServerTest::testBLOB()
{
	const std::size_t maxFldSize = 250000;
	session().setProperty("maxFieldSize", Poco::Any(maxFldSize-1));
	recreatePersonBLOBTable();

	try
	{
		executor().blob(maxFldSize);
		fail ("must fail");
	}
	catch (DataException&)
	{
		session().setProperty("maxFieldSize", Poco::Any(maxFldSize));
	}

	for (int i = 0; i < 8;)
	{
		recreatePersonBLOBTable();
		session().setFeature("autoBind", bindValue(i));
		session().setFeature("autoExtract", bindValue(i+1));
		executor().blob(maxFldSize);
		i += 2;
	}

	recreatePersonBLOBTable();
	try
	{
		executor().blob(maxFldSize+1);
		fail ("must fail");
	}
	catch (DataException&) { }
}


void ODBCSQLServerTest::testNull()
{
	// test for NOT NULL violation exception
	for (int i = 0; i < 8;)
	{
		recreateNullsTable("NOT NULL");
		session().setFeature("autoBind", bindValue(i));
		session().setFeature("autoExtract", bindValue(i+1));
		executor().notNulls("23000");
		i += 2;
	}

	// test for null insertion
	for (int i = 0; i < 8;)
	{
		recreateNullsTable();
		session().setFeature("autoBind", bindValue(i));
		session().setFeature("autoExtract", bindValue(i+1));
		executor().nulls();
		i += 2;
	}
}


void ODBCSQLServerTest::testStoredProcedure()
{
	for (int k = 0; k < 8;)
	{
		session().setFeature("autoBind", bindValue(k));
		session().setFeature("autoExtract", bindValue(k+1));

		dropObject("PROCEDURE", "storedProcedure");

		session() << "CREATE PROCEDURE storedProcedure(@outParam int OUTPUT) AS "
			"BEGIN "
			"SET @outParam = -1; "
			"END;"
		, now;
		
		int i = 0;
		session() << "{call storedProcedure(?)}", out(i), now;
		assert(-1 == i);
		dropObject("PROCEDURE", "storedProcedure");

		session() << "CREATE PROCEDURE storedProcedure(@inParam int, @outParam int OUTPUT) AS "
			"BEGIN "
			"SET @outParam = @inParam*@inParam; "
			"END;"
		, now;

		i = 2;
		int j = 0;
		session() << "{call storedProcedure(?, ?)}", in(i), out(j), now;
		assert(4 == j);
		dropObject("PROCEDURE", "storedProcedure");

		session() << "CREATE PROCEDURE storedProcedure(@ioParam int OUTPUT) AS "
			"BEGIN "
			"SET @ioParam = @ioParam*@ioParam; "
			"END;"
		, now;

		i = 2;
		session() << "{call storedProcedure(?)}", io(i), now;
		assert(4 == i);
		dropObject("PROCEDURE", "storedProcedure");

		session() << "CREATE PROCEDURE storedProcedure(@ioParam DATETIME OUTPUT) AS "
			"BEGIN "
			" SET @ioParam = @ioParam + 1; "
			"END;" , now;

		DateTime dt(1965, 6, 18, 5, 35, 1);
		session() << "{call storedProcedure(?)}", io(dt), now;
		assert(19 == dt.day());
		dropObject("PROCEDURE", "storedProcedure");

		k += 2;
	}
/*TODO - currently fails with following error:

[Microsoft][ODBC SQL Server Driver][SQL Server]Invalid parameter 
2 (''):  Data type 0x23 is a deprecated large object, or LOB, but is marked as output parameter.  
Deprecated types are not supported as output parameters.  Use current large object types instead.

	session().setFeature("autoBind", true);
	session() << "CREATE PROCEDURE storedProcedure(@inParam VARCHAR(MAX), @outParam VARCHAR(MAX) OUTPUT) AS "
		"BEGIN "
		"SET @outParam = @inParam; "
		"END;"
	, now;

	std::string inParam = "123";
	std::string outParam;
	try{
	session() << "{call storedProcedure(?, ?)}", in(inParam), out(outParam), now;
	}catch(StatementException& ex){std::cout << ex.toString();}
	assert(outParam == inParam);
	dropObject("PROCEDURE", "storedProcedure");
	*/
}


void ODBCSQLServerTest::testCursorStoredProcedure()
{
	for (int k = 0; k < 8;)
	{
		session().setFeature("autoBind", bindValue(k));
		session().setFeature("autoExtract", bindValue(k+1));

		recreatePersonTable();
		typedef Tuple<std::string, std::string, std::string, int> Person;
		std::vector<Person> people;
		people.push_back(Person("Simpson", "Homer", "Springfield", 42));
		people.push_back(Person("Simpson", "Bart", "Springfield", 12));
		people.push_back(Person("Simpson", "Lisa", "Springfield", 10));
		session() << "INSERT INTO Person VALUES (?, ?, ?, ?)", use(people), now;

		dropObject("PROCEDURE", "storedCursorProcedure");
		session() << "CREATE PROCEDURE storedCursorProcedure(@ageLimit int) AS "
			"BEGIN "
			" SELECT * "
			" FROM Person "
			" WHERE Age < @ageLimit " 
			" ORDER BY Age DESC; "
			"END;"
		, now;

		people.clear();
		int age = 13;
		
		session() << "{call storedCursorProcedure(?)}", in(age), into(people), now;
		
		assert (2 == people.size());
		assert (Person("Simpson", "Bart", "Springfield", 12) == people[0]);
		assert (Person("Simpson", "Lisa", "Springfield", 10) == people[1]);

		Statement stmt = ((session() << "{call storedCursorProcedure(?)}", in(age), now));
		RecordSet rs(stmt);
		assert (rs["LastName"] == "Simpson");
		assert (rs["FirstName"] == "Bart");
		assert (rs["Address"] == "Springfield");
		assert (rs["Age"] == 12);

		dropObject("TABLE", "Person");
		dropObject("PROCEDURE", "storedCursorProcedure");

		k += 2;
	}
}


void ODBCSQLServerTest::testStoredProcedureAny()
{
	for (int k = 0; k < 8;)
	{
		session().setFeature("autoBind", bindValue(k));
		session().setFeature("autoExtract", bindValue(k+1));

		Any i = 2;
		Any j = 0;

		session() << "CREATE PROCEDURE storedProcedure(@inParam int, @outParam int OUTPUT) AS "
			"BEGIN "
			"SET @outParam = @inParam*@inParam; "
			"END;"
		, now;

		session() << "{call storedProcedure(?, ?)}", in(i), out(j), now;
		assert(4 == AnyCast<int>(j));
		session() << "DROP PROCEDURE storedProcedure;", now;

		session() << "CREATE PROCEDURE storedProcedure(@ioParam int OUTPUT) AS "
			"BEGIN "
			"SET @ioParam = @ioParam*@ioParam; "
			"END;"
		, now;

		i = 2;
		session() << "{call storedProcedure(?)}", io(i), now;
		assert(4 == AnyCast<int>(i));
		dropObject("PROCEDURE", "storedProcedure");

		k += 2;
	}
}


void ODBCSQLServerTest::testStoredProcedureDynamicAny()
{
	for (int k = 0; k < 8;)
	{
		session().setFeature("autoBind", bindValue(k));
		
		DynamicAny i = 2;
		DynamicAny j = 0;

		session() << "CREATE PROCEDURE storedProcedure(@inParam int, @outParam int OUTPUT) AS "
			"BEGIN "
			"SET @outParam = @inParam*@inParam; "
			"END;"
		, now;

		session() << "{call storedProcedure(?, ?)}", in(i), out(j), now;
		assert(4 == j);
		session() << "DROP PROCEDURE storedProcedure;", now;

		session() << "CREATE PROCEDURE storedProcedure(@ioParam int OUTPUT) AS "
			"BEGIN "
			"SET @ioParam = @ioParam*@ioParam; "
			"END;"
		, now;

		i = 2;
		session() << "{call storedProcedure(?)}", io(i), now;
		assert(4 == i);
		dropObject("PROCEDURE", "storedProcedure");

		k += 2;
	}
}


void ODBCSQLServerTest::testStoredFunction()
{
	for (int k = 0; k < 8;)
	{
		session().setFeature("autoBind", bindValue(k));
		session().setFeature("autoExtract", bindValue(k+1));

		dropObject("PROCEDURE", "storedFunction");
		session() << "CREATE PROCEDURE storedFunction AS "
			"BEGIN "
			"DECLARE @retVal int;"
			"SET @retVal = -1;"
			"RETURN @retVal;"
			"END;"
		, now;

		int i = 0;
		session() << "{? = call storedFunction}", out(i), now;
		assert(-1 == i);
		dropObject("PROCEDURE", "storedFunction");


		session() << "CREATE PROCEDURE storedFunction(@inParam int) AS "
			"BEGIN "
			"RETURN @inParam*@inParam;"
			"END;"
		, now;

		i = 2;
		int result = 0;
		session() << "{? = call storedFunction(?)}", out(result), in(i), now;
		assert(4 == result);
		dropObject("PROCEDURE", "storedFunction");


		session() << "CREATE PROCEDURE storedFunction(@inParam int, @outParam int OUTPUT) AS "
			"BEGIN "
			"SET @outParam = @inParam*@inParam;"
			"RETURN @outParam;"
			"END"
		, now;

		i = 2;
		int j = 0;
		result = 0;
		session() << "{? = call storedFunction(?, ?)}", out(result), in(i), out(j), now;
		assert(4 == j);
		assert(j == result);
		dropObject("PROCEDURE", "storedFunction");


		session() << "CREATE PROCEDURE storedFunction(@param1 int OUTPUT,@param2 int OUTPUT) AS "
			"BEGIN "
			"DECLARE @temp int; "
			"SET @temp = @param1;"
			"SET @param1 = @param2;"
			"SET @param2 = @temp;"
			"RETURN @param1 + @param2; "
			"END"
		, now;

		i = 1;
		j = 2;
		result = 0;
		session() << "{? = call storedFunction(?, ?)}", out(result), io(i), io(j), now;
		assert(1 == j);
		assert(2 == i);
		assert(3 == result); 

		Tuple<int, int> params(1, 2);
		assert(1 == params.get<0>());
		assert(2 == params.get<1>());
		result = 0;
		session() << "{? = call storedFunction(?, ?)}", out(result), io(params), now;
		assert(1 == params.get<1>());
		assert(2 == params.get<0>());
		assert(3 == result); 

		dropObject("PROCEDURE", "storedFunction");

		k += 2;
	}
}


void ODBCSQLServerTest::dropObject(const std::string& type, const std::string& name)
{
	try
	{
		session() << format("DROP %s %s", type, name), now;
	}
	catch (StatementException& ex)
	{
		bool ignoreError = false;
		const StatementDiagnostics::FieldVec& flds = ex.diagnostics().fields();
		StatementDiagnostics::Iterator it = flds.begin();
		for (; it != flds.end(); ++it)
		{
			if (3701 == it->_nativeError)//(table does not exist)
			{
				ignoreError = true;
				break;
			}
		}

		if (!ignoreError) throw;
	}
}


void ODBCSQLServerTest::recreatePersonTable()
{
	dropObject("TABLE", "Person");
	try { session() << "CREATE TABLE Person (LastName VARCHAR(30), FirstName VARCHAR(30), Address VARCHAR(30), Age INTEGER)", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreatePersonTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreatePersonTable()"); }
}


void ODBCSQLServerTest::recreatePersonBLOBTable()
{
	dropObject("TABLE", "Person");
	try { session() << "CREATE TABLE Person (LastName VARCHAR(30), FirstName VARCHAR(30), Address VARCHAR(30), Image VARBINARY(MAX))", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreatePersonBLOBTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreatePersonBLOBTable()"); }
}


void ODBCSQLServerTest::recreatePersonDateTimeTable()
{
	dropObject("TABLE", "Person");
	try { session() << "CREATE TABLE Person (LastName VARCHAR(30), FirstName VARCHAR(30), Address VARCHAR(30), Born DATETIME)", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreatePersonDateTimeTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreatePersonDateTimeTable()"); }
}


void ODBCSQLServerTest::recreateIntsTable()
{
	dropObject("TABLE", "Strings");
	try { session() << "CREATE TABLE Strings (str INTEGER)", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateIntsTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateIntsTable()"); }
}


void ODBCSQLServerTest::recreateStringsTable()
{
	dropObject("TABLE", "Strings");
	try { session() << "CREATE TABLE Strings (str VARCHAR(30))", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateStringsTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateStringsTable()"); }
}


void ODBCSQLServerTest::recreateFloatsTable()
{
	dropObject("TABLE", "Strings");
	try { session() << "CREATE TABLE Strings (str FLOAT)", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateFloatsTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateFloatsTable()"); }
}


void ODBCSQLServerTest::recreateTuplesTable()
{
	dropObject("TABLE", "Tuples");
	try { session() << "CREATE TABLE Tuples "
		"(int0 INTEGER, int1 INTEGER, int2 INTEGER, int3 INTEGER, int4 INTEGER, int5 INTEGER, int6 INTEGER, "
		"int7 INTEGER, int8 INTEGER, int9 INTEGER, int10 INTEGER, int11 INTEGER, int12 INTEGER, int13 INTEGER,"
		"int14 INTEGER, int15 INTEGER, int16 INTEGER, int17 INTEGER, int18 INTEGER, int19 INTEGER)", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateTuplesTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateTuplesTable()"); }
}


void ODBCSQLServerTest::recreateVectorTable()
{
	dropObject("TABLE", "Vector");
	try { session() << "CREATE TABLE Vector (i0 INTEGER)", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateVectorTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateVectorTable()"); }
}


void ODBCSQLServerTest::recreateVectorsTable()
{
	dropObject("TABLE", "Vectors");
	try { session() << "CREATE TABLE Vectors (int0 INTEGER, flt0 FLOAT, str0 VARCHAR(30))", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateVectorsTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateVectorsTable()"); }
}


void ODBCSQLServerTest::recreateAnysTable()
{
	dropObject("TABLE", "Anys");
	try { session() << "CREATE TABLE Anys (int0 INTEGER, flt0 FLOAT, str0 VARCHAR(30))", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateAnysTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateAnysTable()"); }
}


void ODBCSQLServerTest::recreateNullsTable(const std::string& notNull)
{
	dropObject("TABLE", "NullTest");
	try { session() << format("CREATE TABLE NullTest (i INTEGER %s, r FLOAT %s, v VARCHAR(30) %s)",
		notNull,
		notNull,
		notNull), now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateNullsTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateNullsTable()"); }
}

void ODBCSQLServerTest::recreateBoolTable()
{
	dropObject("TABLE", "BoolTest");
	try { session() << "CREATE TABLE BoolTest (b BIT)", now; }
	catch(ConnectionException& ce){ std::cout << ce.toString() << std::endl; fail ("recreateBoolTable()"); }
	catch(StatementException& se){ std::cout << se.toString() << std::endl; fail ("recreateBoolTable()"); }
}


CppUnit::Test* ODBCSQLServerTest::suite()
{
	if (_pSession = init(_driver, _dsn, _uid, _pwd, _connectString, _db))
	{
		std::cout << "*** Connected to [" << _driver << "] test database." << std::endl;

		_pExecutor = new SQLExecutor(_driver + " SQL Executor", _pSession);

		CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("ODBCSQLServerTest");

		CppUnit_addTest(pSuite, ODBCSQLServerTest, testBareboneODBC);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSimpleAccess);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testComplexType);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSimpleAccessVector);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testComplexTypeVector);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInsertVector);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInsertEmptyVector);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSimpleAccessList);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testComplexTypeList);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInsertList);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInsertEmptyList);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSimpleAccessDeque);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testComplexTypeDeque);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInsertDeque);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInsertEmptyDeque);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInsertSingleBulk);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInsertSingleBulkVec);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testLimit);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testLimitOnce);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testLimitPrepare);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testLimitZero);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testPrepare);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testStep);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSetSimple);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSetComplex);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSetComplexUnique);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testMultiSetSimple);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testMultiSetComplex);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testMapComplex);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testMapComplexUnique);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testMultiMapComplex);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSelectIntoSingle);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSelectIntoSingleStep);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSelectIntoSingleFail);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testLowerLimitOk);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testLowerLimitFail);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testCombinedLimits);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testCombinedIllegalLimits);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testRange);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testIllegalRange);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testSingleSelect);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testEmptyDB);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testBLOB);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testBLOBStmt);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testDateTime);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testFloat);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testDouble);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testTuple);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testTupleVector);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testStoredProcedure);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testCursorStoredProcedure);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testStoredProcedureAny);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testStoredProcedureDynamicAny);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testStoredFunction);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInternalExtraction);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testInternalStorageType);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testNull);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testRowIterator);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testStdVectorBool);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testAsync);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testAny);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testDynamicAny);
		CppUnit_addTest(pSuite, ODBCSQLServerTest, testMultipleResults);

		return pSuite;
	}

	return 0;
}

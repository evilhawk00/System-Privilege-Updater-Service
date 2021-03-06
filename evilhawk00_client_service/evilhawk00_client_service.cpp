//https://github.com/evilhawk00/System-Privilege-Updater-Service
//
//System Privilege Updater Service
//
//Copyright © 2018 CHEN-YAN-LIN
//
//A simple example for Creating a Windows Service that performs a software update with SYSTEM privilege at startup.
//With this example, software publisher can ensure their software are always up - to - date on some PCs even if the Logged in user is not a system administrator.
//This concept is also used on some famous software, such as, Steam by Valve Corporation, even user logged in without administrator privilege, they can always keep their installed games up - to - date.

#include "stdafx.h"



#include <tchar.h>
#include <process.h>
#include <winsock2.h>
#include <Winhttp.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <ctype.h>
#include <vector>
#include <io.h>
#include <time.h>
#include <wchar.h>
#include <cstdlib>
#include <atlstr.h>
#include <Netlistmgr.h>
using namespace std;
#pragma comment(lib, "Winhttp.lib")

//modify the wstring here to customize the settings
//*SETTINGS SRARTS HERE*//
#define LocalConfigFilePATH L"C:\\Program Files\\SampleUpdateService\\SampleUpdateSrvRecord.dat"   //PATH to the local setting file
#define LocalFileRemovalQueuePATH L"C:\\Program Files\\SampleUpdateService\\depricated.dat" //PATH to local record of file for pending removal
#define SelfUpgradeOldBakBin L"C:\\Program Files\\SampleUpdateService\\SamUdSrv.exe.Bak"  //PATH to renamed exe when self upgrade executed
#define MainUpdateHostServer L"myhostname.abcdefgblablalba.com"  //main update server
#define MainUpdateMetaLink L"/UpdateService/Update.ini" //link path to the remote metadata file right after the host name
#define SERVICE_NAME  L"evilhawk00UpdateService" //service name
//*SETTINGS ENDS HERE*//


typedef map<std::string, std::string> ConfigData;
ConfigData ConfigValues;
ConfigData LocalConfigs;

typedef map<std::wstring, std::wstring> RemovalQueue;
RemovalQueue LocalRMQueue;
//initialize variable for remote command
bool COMMAND_LINE = FALSE;
bool DOWNLOAD_STOREONLY_MODE = FALSE;
bool RESET_VERSION_COUNT = FALSE;
bool DOWNLOAD_FROM_EXTHOST = FALSE;
bool SELF_UPGRADE_MODE = FALSE;
//initialize variable for local metadata result
bool LOCAL_VERNUM_EXIST = FALSE;
bool LOCAL_RESETID_EXIST = FALSE;
//flag for deciding to do a update action or not
bool UPDATE_OR_NOT = FALSE;


//define wstring for tmp folder value
wstring TempPath; //system Temp folder environment variable
wstring RandomTMPFolder;  //generated randmom folder name,ex: tmp001A2B3C4
wstring TMPFldPATH;
wstring MetaDataTMPPATH;
wstring RandomDatName;
wstring RanDatPATH;
wstring RanBinSTR;
wstring RandomBinName;
wstring RanBinPATH;
wstring PendingRMfldPATH;
wstring PendingRMbinPATH;

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE ServiceStatusHandle = NULL;


VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);







//random tmp folder name generator with hex value
static const char RandomHexPool[] =
"0123456789"
"ABCDEF";

int RanPooolSize = sizeof(RandomHexPool) - 1;


wchar_t GetRamdomChar() {
	return RandomHexPool[rand() % RanPooolSize];
}


wstring GenerateRamdomTMPstr() {
	//srand((unsigned int)time(0));
	wstring RamdomFld = L"tmp00";
	for (int i = 0; i < 7; i++) {
		RamdomFld += GetRamdomChar();
	}

	return RamdomFld;
}

wstring GenerateRamdomDatName() {
	//srand((unsigned int)time(0));
	wstring RanDatName = L"~";
	for (int i = 0; i < 8; i++) {
		RanDatName += GetRamdomChar();
	}

	return RanDatName;
}

wstring GenerateRamdomBinName() {
	//srand((unsigned int)time(0));
	wstring RanBinName = L"tmp";
	for (int i = 0; i < 4; i++) {
		RanBinName += GetRamdomChar();
	}

	return RanBinName;
}



void PrepareTMPfldVariable() {
	//Run Ramdom seed
	srand((unsigned int)time(0));
	//get system TEMP environment variable
	wchar_t wchPath[MAX_PATH];
	if (GetTempPathW(MAX_PATH, wchPath) != 0) {
		//success, set TempPath(wstring) as system TEMP Path
		TempPath = wchPath;
		//generate tmpfolder string for pre-execution
		RandomTMPFolder = GenerateRamdomTMPstr();
		TMPFldPATH = TempPath + RandomTMPFolder + L"\\";
		MetaDataTMPPATH = TMPFldPATH + L"MetaData";
	}
	else {
		//error on retreveing tmp path, using c:\windows\temp\ as temp PATH
		//generate tmpfolder string for pre-execution
		TempPath = L"C:\\Windows\\Temp\\";
		RandomTMPFolder = GenerateRamdomTMPstr();
		//Generate full PATH to tmp folder(wstring)
		TMPFldPATH = L"C:\\Windows\\Temp\\" + RandomTMPFolder + L"\\";
		MetaDataTMPPATH = TMPFldPATH + L"MetaData";

		//create c:\windows\temp\ if it did not exist
		if ((_access("C:\\Windows\\Temp\\", 0)) == -1) {
			//cout << "directory not exist" << endl;
			CreateDirectory(L"C:\\Windows\\Temp\\", NULL);
			//cout << "directory created" << endl;
		}

	}

}

void PrepareRanBinVariable() {
	//generate random dat string for pre-execution
	wstring RanDatSTR = GenerateRamdomDatName();
	RandomDatName = RanDatSTR + L".dat";
	//Generate full PATH to Random binary(wstring)
	RanDatPATH = TMPFldPATH + RandomDatName;

	RanBinSTR = GenerateRamdomBinName();
	RandomBinName = RanBinSTR + L".exe";
	RanBinPATH = TMPFldPATH + RandomBinName;

}


inline bool isInteger(const string & s)
{
	if (s.find_first_not_of("0123456789")) {
		//first character is matching 0-9
		char* p;
		strtod(s.c_str(), &p);
		return *p == 0;
	}
	else {

		return false;
	}



}

wstring s2ws(const string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	vector<wchar_t> buf(len);
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf.data(), len);
	return buf.data();
}






int DownloadMetaData() {
	//return -1 if any I/O error, return -2 if any network error
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	DWORD dwStatusCode = 0;
	LPBYTE pszOutBuffer;

	BOOL  bResults = FALSE, bResponse = FALSE;
	HINTERNET  hSession = NULL, hConnect = NULL, hRequest = NULL;

	wstring UserAgent;
	if (LOCAL_VERNUM_EXIST) {
		// if version variable was found
		UserAgent = s2ws("evilhawk00 OpenSource SampleUpdateService/1.0b ver:" + LocalConfigs["VER"]);

	}
	else {
		//Use user-agent without version
		UserAgent = L"evilhawk00 OpenSource SampleUpdateService/1.0b NoVerInfo";
	}



	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen(UserAgent.c_str(), WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	// Specify an HTTP server.
	if (hSession) {
		hConnect = WinHttpConnect(hSession, MainUpdateHostServer, INTERNET_DEFAULT_HTTP_PORT, 0);
	}
	else {
		return -2;
	}
	// Create an HTTP request handle.
	if (hConnect) {
		hRequest = WinHttpOpenRequest(hConnect, L"GET", MainUpdateMetaLink, NULL, WINHTTP_NO_REFERER, NULL, 0);
	}
	else {
		if (hSession) WinHttpCloseHandle(hSession);
		return -2;
	}
	// Send a request.
	if (hRequest) {
		bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	}
	else {
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
		return -2;
	}

	// End the request.
	if (bResults) {
		bResponse = WinHttpReceiveResponse(hRequest, NULL);
	}
	else {
		if (hRequest) WinHttpCloseHandle(hRequest);
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
		return -2;
	}

	if (bResponse) {
		//connection success, analysing HTTP Status Code
		dwSize = sizeof(dwStatusCode);
		WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

		if (dwStatusCode == HTTP_STATUS_OK) {
			//200 OK, Attempting to write data into harddrive

			int CreateTMPfld = CreateDirectory(TMPFldPATH.c_str(), NULL);

			if (CreateTMPfld == 0) {
				//create folder failed,abort
				return -1;
			}




			HANDLE hFile = CreateFile(MetaDataTMPPATH.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				//create file handle error




				return -1;
			}
			else {
				do
				{
					// Check for available data.        
					dwSize = 0;
					if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
						printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
					// Allocate space for the buffer.        
					pszOutBuffer = new byte[dwSize + 1];
					if (!pszOutBuffer)
					{
						printf("Out of memory\n");
						dwSize = 0;
					}
					else
					{
						// Read the Data.            
						ZeroMemory(pszOutBuffer, dwSize + 1);
						if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
						{
							printf("Error %u in WinHttpReadData.\n", GetLastError());
						}
						else
						{
							//printf("%s", pszOutBuffer); 
							DWORD wmWritten;
							WriteFile(hFile, pszOutBuffer, dwSize, &wmWritten, NULL);
							int n = GetLastError();
						}
						// Free the memory allocated to the buffer.            
						delete[] pszOutBuffer;
					}
				} while (dwSize > 0);
				CloseHandle(hFile);

			}
		}
		else {
			//404 NOT FOUND, abort
			if (hRequest) WinHttpCloseHandle(hRequest);
			if (hConnect) WinHttpCloseHandle(hConnect);
			if (hSession) WinHttpCloseHandle(hSession);
			return -1;
		}





	}
	else {
		// the connection was not success, no data received.
		if (hRequest) WinHttpCloseHandle(hRequest);
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
		return -2;

	}



	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);
	return 0;


}




int ReadDownloadedMetaData() {

	ifstream ApplicationMetaData(MetaDataTMPPATH, ios::in);
	if (ApplicationMetaData) {
		//open file success
		string line;
		while (getline(ApplicationMetaData, line)) {
			istringstream lineStream(line);
			string KEY;
			if (getline(lineStream, KEY, '=')) {
				string value;
				if (getline(lineStream, value)) {
					ConfigValues[KEY] = value;
				}
			}
		}
	}
	else {
		//read config file failed
		return -1;
	}
	ApplicationMetaData.close();

	//check if all necessary key value is set

	if (ConfigValues.find("Version") == ConfigValues.end()) {
		//Necessary Version key or value not exist
		return -2;
	}
	else {
		//Version Key Exists, Now check the key format, check if its a valid integer
		if (!isInteger(ConfigValues["Version"])) {
			return -2;
		}

	}

	if (ConfigValues.find("Path") == ConfigValues.end() && (ConfigValues.find("ExtHost") == ConfigValues.end() || ConfigValues.find("ExtPath") == ConfigValues.end())) {
		//Necessary Path key and (ExtHost ExtPath) key both not exist
		return -2;
	}
	else {
		//either Path variable exist or ExtHost&ExtPath variable exist or all exist

		if (ConfigValues.find("Path") != ConfigValues.end()) {
			//Internal Path Variable exist, ExtHost&ExtPath may exist or not
			if (ConfigValues["Path"] == "0") {

				if (ConfigValues.find("ExtHost") != ConfigValues.end() && ConfigValues.find("ExtPath") != ConfigValues.end()) {
					//Internal Path is set to 0, ExtHost and ExtPath both exist
					if (ConfigValues["ExtHost"] != "0" && ConfigValues["ExtPath"] != "0") {
						//ExtHost and ExtPath are both not set to 0,Extneral Host Download Mode Activated
						DOWNLOAD_FROM_EXTHOST = TRUE;


					}
					else {
						//Internal Path is set to 0, both ExtHost and ExtPath are 0, too, config invalid
						return -2;
					}
				}
				else {
					//Internal Path is set to 0, but ExtHost or ExtPath not exist,config invalid
					return -2;
				}

			}

		}
		else {
			//Internal Path Variable not exist, meaning ExtHost Variable and ExtPath Variable both exist
			if (ConfigValues["ExtHost"] != "0" && ConfigValues["ExtPath"] != "0") {
				//ExtHost and ExtPath are both not set to 0,Extneral Host Download Mode Activated
				DOWNLOAD_FROM_EXTHOST = TRUE;
			}
			else {
				//Internal Path variable not exist, ExtHost or ExtPath os set to 0, config is invalid
				return -2;
			}

		}



	}


	//check if any optional key is set

	if (ConfigValues.find("CmdLine") != ConfigValues.end()) {
		//CmdLine key and value exist
		if (ConfigValues["CmdLine"] != "0") {
			//CmdLine is not set to 0, CmdLine is set
			COMMAND_LINE = TRUE;
		}
	}

	if (ConfigValues.find("DLStoreMode") != ConfigValues.end() && ConfigValues.find("DLStoreDir") != ConfigValues.end() && ConfigValues.find("DLStoreFileName") != ConfigValues.end()) {
		//DLStoreMode key and DLStoreDir key and DLStoreFileName key all exist
		if (ConfigValues["DLStoreMode"] == "1" && ConfigValues["DLStoreDir"] != "0" && ConfigValues["DLStoreFileName"] != "0") {
			//DLStoreMode is set to 1, DLStoreFullPath is not set to 0, DLStoreFileName is not set to 0, Download and store mode is set
			DOWNLOAD_STOREONLY_MODE = TRUE;
		}
	}

	if (ConfigValues.find("ResetVersion") != ConfigValues.end() && ConfigValues.find("ResetUniqueID") != ConfigValues.end()) {
		//ResetVersion key and ResetUniqueID key value exist
		if (ConfigValues["ResetVersion"] == "1" && ConfigValues["ResetUniqueID"] != "0") {
			//ResetVersion is set to 1 and ResetUniqueID is not 0, Reset Local Version Variable is activated
			RESET_VERSION_COUNT = TRUE;
		}
	}

	//Check if self upgrade mode is set
	if (ConfigValues.find("SelfUpgrade") != ConfigValues.end()) {
		//SelfUpgrade key and value exist
		if (ConfigValues["SelfUpgrade"] == "1") {
			//SelfUpgrade key is set to 1, SelfUpgrade mode is activated
			SELF_UPGRADE_MODE = TRUE;
		}
	}



	return 0;
}


int WriteMetaDataToLocal() {

	//write version
	//write resetuniqueid

	
	wstring wsFullPath = LocalConfigFilePATH;
	size_t DelimiterPos = wsFullPath.find_last_of(L"\\");
	if (DelimiterPos == wstring::npos) {  // last slash not found
		return -1;

	}

	wstring wsParentDir = wsFullPath.substr(0, DelimiterPos+1);
	//check parent directory existense before write file into it
	if ((_waccess(wsParentDir.c_str(), 0)) == -1) {
		//the destination directory not exist,create it
		wchar_t* path = const_cast<wchar_t*>(wsParentDir.c_str());
		wchar_t folder[MAX_PATH];
		wchar_t* end;
		ZeroMemory(folder, MAX_PATH * sizeof(wchar_t));
		end = wcschr(path, L'\\');
		while (end != NULL)
		{
			wcsncpy_s(folder, path, end - path + 1);
			if (!CreateDirectory(folder, NULL))
			{
				DWORD err = GetLastError();
				if (err != ERROR_ALREADY_EXISTS)
				{
					// do whatever handling you'd like
				}
			}
			end = wcschr(++end, L'\\');
		}
	}

	//open write stream
	ofstream LocalConfig(LocalConfigFilePATH);

	string VersionNUM;
	string ResetUniqueID;
	if (RESET_VERSION_COUNT) {
		VersionNUM = "VER=0";
		ResetUniqueID = "ResetID=" + ConfigValues["ResetUniqueID"];
	}
	else {
		VersionNUM = "VER=" + ConfigValues["Version"];
	}


	if (LocalConfig) {

		LocalConfig << VersionNUM << endl;
		if (RESET_VERSION_COUNT) {
			LocalConfig << ResetUniqueID << endl;
		}

		LocalConfig.close();
	}
	else {
		cout << "Error opening file for output" << endl;
		return -1;
	}
	return 0;
}

int WriteFileRemovalQueue() {

	//write random tmpfld string
	//write random bin string

	wstring wsFullPath = LocalFileRemovalQueuePATH;
	size_t DelimiterPos = wsFullPath.find_last_of(L"\\");
	if (DelimiterPos == wstring::npos) {  // last slash not found
		return -1;

	}

	wstring wsParentDir = wsFullPath.substr(0, DelimiterPos + 1);
	//check parent directory existense before write file into it
	if ((_waccess(wsParentDir.c_str(), 0)) == -1) {
		//the destination directory not exist,create it
		wchar_t* path = const_cast<wchar_t*>(wsParentDir.c_str());
		wchar_t folder[MAX_PATH];
		wchar_t* end;
		ZeroMemory(folder, MAX_PATH * sizeof(wchar_t));
		end = wcschr(path, L'\\');
		while (end != NULL)
		{
			wcsncpy_s(folder, path, end - path + 1);
			if (!CreateDirectory(folder, NULL))
			{
				DWORD err = GetLastError();
				if (err != ERROR_ALREADY_EXISTS)
				{
					// do whatever handling you'd like
				}
			}
			end = wcschr(++end, L'\\');
		}
	}

	wofstream NewRemoveListQueue(LocalFileRemovalQueuePATH);

	wstring RandomTmpfldList;
	wstring RandomBinList;

	RandomTmpfldList = L"FLD=" + RandomTMPFolder;
	RandomBinList = L"BIN=" + RanBinSTR;



	if (NewRemoveListQueue) {

		NewRemoveListQueue << RandomTmpfldList << endl;
		NewRemoveListQueue << RandomBinList << endl;


		NewRemoveListQueue.close();
	}
	else {
		cout << "Error opening file for output" << endl;
		return -1;
	}
	return 0;
}

int ReadFileRemovalQueue() {
	wifstream RMQueueList(LocalFileRemovalQueuePATH, ios::in);
	if (RMQueueList) {
		//open file success
		wstring line;
		while (getline(RMQueueList, line)) {
			wistringstream lineStream(line);
			wstring KEY;
			if (getline(lineStream, KEY, L'=')) {
				wstring value;
				if (getline(lineStream, value)) {
					LocalRMQueue[KEY] = value;
				}
			}
		}
	}
	else {
		//read removal queue file failed
		//cout << "Error opening input file" << endl;
		return -1;
	}
	RMQueueList.close();

	if (LocalRMQueue.find(L"FLD") != LocalRMQueue.end() && LocalRMQueue.find(L"BIN") != LocalRMQueue.end()) {
		//Local FLD key and BIN key value pairs both exist,continue to set variable and delete remains


		PendingRMfldPATH = TempPath + LocalRMQueue[L"FLD"] + L"\\";
		PendingRMbinPATH = PendingRMfldPATH + LocalRMQueue[L"BIN"] + L".exe";


	}
	else {
		// Local FLD key and BIN key value pairs not exist,abort
		return -2;
	}



	return 0;
	//cout << "ver" + LocalConfigs["VER"] << endl;


}

int DeleteOldRemainFiles() {
	bool DELETE_SUCCESS = TRUE;


	if ((_waccess(PendingRMbinPATH.c_str(), 0)) != -1) {
		// pending removal .exe exist
		if ((DeleteFile(PendingRMbinPATH.c_str())) == 0)
		{
			//deleting remain exe failed
			DELETE_SUCCESS = FALSE;
		}

	}

	if ((_waccess(PendingRMfldPATH.c_str(), 0)) != -1) {
		// pending removal tmp folder exist
		Sleep(1000); // wait 1 sec
		if ((RemoveDirectory(PendingRMfldPATH.c_str())) == 0)
		{
			//Removing remain temp folder was failed
			DELETE_SUCCESS = FALSE;
		}

	}

	if (!DELETE_SUCCESS) {
		// Error occured during the deletion
		return -1;
	}


	return 0;

}

int DeleteRemovalQueueFile() {

	DeleteFile(LocalFileRemovalQueuePATH);

	return 0;


}

int ReadLocalMetaData() {
	ifstream LocalMetaData(LocalConfigFilePATH, ios::in);
	if (LocalMetaData) {
		//open file success
		string line;
		while (getline(LocalMetaData, line)) {
			istringstream lineStream(line);
			string KEY;
			if (getline(lineStream, KEY, '=')) {
				string value;
				if (getline(lineStream, value)) {
					LocalConfigs[KEY] = value;
				}
			}
		}
	}
	else {
		//read config file failed
		//cout << "Error opening input file" << endl;
		return -1;
	}
	LocalMetaData.close();

	if (LocalConfigs.find("VER") != LocalConfigs.end()) {
		//Local VER key and value exist,continue to validate local VER value
		if (!isInteger(LocalConfigs["VER"])) {
			//Local VER key value invalid
			return -2;
		}
		else {
			LOCAL_VERNUM_EXIST = TRUE;
		}
	}
	else {
		// Local VER key not exist,abort
		return -2;
	}

	if (LocalConfigs.find("ResetID") != LocalConfigs.end()) {
		//Local ResetID key and value exist
		LOCAL_RESETID_EXIST = TRUE;
	}

	return 0;
	//cout << "ver" + LocalConfigs["VER"] << endl;


}


int CompareRemoteLocalVer() {


	unsigned int LocalVersionNum;
	unsigned int RemoteVersionNum;
	//try converting grabbed string value to int

	try {
		LocalVersionNum = stoi(LocalConfigs["VER"]);
	}
	catch (std::invalid_argument) {
		return -1;
	}
	catch (std::out_of_range) {
		return -1;
	}

	try {
		RemoteVersionNum = stoi(ConfigValues["Version"]);
	}
	catch (std::invalid_argument) {
		return -1;
	}
	catch (std::out_of_range) {
		return -1;
	}


	if (RemoteVersionNum > LocalVersionNum) {
		//remote is newer than local version
		UPDATE_OR_NOT = TRUE;
	}

	return 0;
}





int DownloadUpdateBinary() {
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	DWORD dwStatusCode = 0;
	LPBYTE pszOutBuffer;

	BOOL  bResults = FALSE, bResponse = FALSE;
	HINTERNET  hSession = NULL, hConnect = NULL, hRequest = NULL;
	wstring UserAgent;
	wstring HostName;
	wstring FilePath;
	if (LOCAL_VERNUM_EXIST) {
		// if version variable was found
		UserAgent = s2ws("evilhawk00 OpenSource SampleUpdateService/1.0b ver:" + LocalConfigs["VER"]);

	}
	else {
		//Use user-agent without version
		UserAgent = L"evilhawk00 OpenSource SampleUpdateService/1.0b NoVerInfo";
	}

	if (DOWNLOAD_FROM_EXTHOST) {
		//Download from external host is triggered
		HostName = s2ws(ConfigValues["ExtHost"]);
		FilePath = s2ws(ConfigValues["ExtPath"]);
	}
	else {
		//download from internal host
		HostName = MainUpdateHostServer;
		FilePath = s2ws(ConfigValues["Path"]);
	}

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen(UserAgent.c_str(), WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	// Specify an HTTP server.
	if (hSession) {
		hConnect = WinHttpConnect(hSession, HostName.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
	}
	else {
		return -1;
	}
	// Create an HTTP request handle.
	if (hConnect) {
		hRequest = WinHttpOpenRequest(hConnect, L"GET", FilePath.c_str(), NULL, WINHTTP_NO_REFERER, NULL, 0);
	}
	else {
		if (hSession) WinHttpCloseHandle(hSession);
		return -1;
	}
	// Send a request.
	if (hRequest) {
		bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	}
	else {
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
		return -1;
	}

	// End the request.
	if (bResults) {
		bResponse = WinHttpReceiveResponse(hRequest, NULL);
	}
	else {
		if (hRequest) WinHttpCloseHandle(hRequest);
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
		return -1;
	}

	if (bResponse) {
		//connection success, analysing HTTP Status Code
		dwSize = sizeof(dwStatusCode);
		WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

		if (dwStatusCode == HTTP_STATUS_OK) {
			//200 OK, Attempting to write data into harddrive

			//wstring BinaryStorePATH = TMPFldPATH + L"2.exe";
			HANDLE hFile = CreateFile(RanDatPATH.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				//create file handle error


				return -1;
			}
			else {
				//create file handle successed
				do
				{
					// Check for available data.        
					dwSize = 0;
					if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
						printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
					// Allocate space for the buffer.        
					pszOutBuffer = new byte[dwSize + 1];
					if (!pszOutBuffer)
					{
						printf("Out of memory\n");
						dwSize = 0;
					}
					else
					{
						// Read the Data.            
						ZeroMemory(pszOutBuffer, dwSize + 1);
						if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
						{
							printf("Error %u in WinHttpReadData.\n", GetLastError());
						}
						else
						{
							//printf("%s", pszOutBuffer); 
							DWORD wmWritten;
							WriteFile(hFile, pszOutBuffer, dwSize, &wmWritten, NULL);

							int n = GetLastError();

						}
						// Free the memory allocated to the buffer.            
						delete[] pszOutBuffer;
					}
				} while (dwSize > 0);
				CloseHandle(hFile);
			}


		}
		else {
			//404 NOT FOUND, abort
			if (hRequest) WinHttpCloseHandle(hRequest);
			if (hConnect) WinHttpCloseHandle(hConnect);
			if (hSession) WinHttpCloseHandle(hSession);
			return -1;
		}





	}
	else {
		// the connection was not success, no data received.
		if (hRequest) WinHttpCloseHandle(hRequest);
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
		return -1;

	}



	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);
	return 0;



}


int RenameDLdata() {


	if (DOWNLOAD_STOREONLY_MODE) {
		// download store mode activated

		wstring StoreFileDIR = s2ws(ConfigValues["DLStoreDir"]);
		wstring StoreFileNAME = s2ws(ConfigValues["DLStoreFileName"]);
		wstring StoreFileFULLPATH = StoreFileDIR + StoreFileNAME;


		//check directory existense before copy
		if ((_waccess(StoreFileDIR.c_str(), 0)) == -1) {
			//the destination directory not exist,create it
			wchar_t *path = const_cast< wchar_t* >(StoreFileDIR.c_str());
			wchar_t folder[MAX_PATH];
			wchar_t *end;
			ZeroMemory(folder, MAX_PATH * sizeof(wchar_t));
			end = wcschr(path, L'\\');
			while (end != NULL)
			{
				wcsncpy_s(folder, path, end - path + 1);
				if (!CreateDirectory(folder, NULL))
				{
					DWORD err = GetLastError();
					if (err != ERROR_ALREADY_EXISTS)
					{
						// do whatever handling you'd like
					}
				}
				end = wcschr(++end, L'\\');
			}
		}

		int MoveStoreFile = MoveFileEx(RanDatPATH.c_str(), StoreFileFULLPATH.c_str(), MOVEFILE_REPLACE_EXISTING);
		if (MoveStoreFile == 0) {
			//rename failed
			return -1;
		}
		int SetAttributeNormal = SetFileAttributes(StoreFileFULLPATH.c_str(), FILE_ATTRIBUTE_NORMAL);
		if (SetAttributeNormal == 0) {
			//set file attribute normal failed
			return -1;
		}

	}
	else {

		// download store mode not activated, proceed renaming and executing

		int RenameBinary = MoveFileEx(RanDatPATH.c_str(), RanBinPATH.c_str(), MOVEFILE_REPLACE_EXISTING);

		if (RenameBinary == 0) {
			//rename failed
			return -1;
		}

	}
	return 0;
}





int ExecDLBin() {

	int exec = 0;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));


	if (COMMAND_LINE) {
		//execution command line parameters is set
		wstring CommandParams;
		CommandParams = s2ws(ConfigValues["CmdLine"]);
		wstring ASpaceANDParams;
		ASpaceANDParams = L" " + CommandParams;
		exec = CreateProcess(RanBinPATH.c_str(), const_cast<LPWSTR>(ASpaceANDParams.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);


	}
	else {
		//execution command parameters is not set

		exec = CreateProcess(RanBinPATH.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);


	}



	if (exec == 0) {
		return -1;
	}

	if (!SELF_UPGRADE_MODE) {
		//self upgrade mode is not set, wait exec to exit
		WaitForSingleObject(pi.hProcess, INFINITE);
	}



	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;

}


int DeleteRanEXE() {
	//delete tmpxxxx.exe
	bool DELETE_SUCCESS = TRUE;


	if ((DeleteFile(RanBinPATH.c_str())) == 0)
	{
		//delete downloaded exe failed
		DELETE_SUCCESS = FALSE;
	}


	if (!DELETE_SUCCESS) {
		// Error occured during the deletion
		return -1;
	}


	return 0;
}

int CheckSelfUpgradeBAK() {

	if ((_waccess(SelfUpgradeOldBakBin, 0)) != -1) {
		//Old exe backcup file exist
		DeleteFile(SelfUpgradeOldBakBin);
	

	}


	return 0;
}

int CleanUpTEMPfld() {

	//delete metadata 
	//delete tmp000folder
	//delete tmpxxxx.exe
	Sleep(1500);
	bool DELETE_SUCCESS = TRUE;

	if ((DeleteFile(MetaDataTMPPATH.c_str())) == 0)
	{
		//delete metadata failed
		DELETE_SUCCESS = FALSE;
	}

	//if self upgrade mode is not activated, cleanup downloaded exe
	if (!SELF_UPGRADE_MODE) {
		if ((DeleteFile(RanBinPATH.c_str())) == 0)
		{
			//delete downloaded exe failed
			DELETE_SUCCESS = FALSE;
		}
	}


	//check existense of downloaded .dat file in case of the error of renaming the file
	if ((_waccess(RanDatPATH.c_str(), 0)) != -1) {
		//.dat file exist
		if ((DeleteFile(RanDatPATH.c_str())) == 0)
		{
			//delete .dat file failed
			DELETE_SUCCESS = FALSE;
		}

	}

	Sleep(1000); // wait 1 sec
	if ((RemoveDirectory(TMPFldPATH.c_str())) == 0)
	{
		//Remove temp folder was failed
		DELETE_SUCCESS = FALSE;
	}



	if (!DELETE_SUCCESS) {
		// Error occured during the deletion
		return -1;
	}


	return 0;
}

int CheckConnectivity() {
	//return 0 if network is connected, return -2 if notwork disconnected, return -1 if function initialize failed
	HRESULT hr = 0;
	VARIANT_BOOL vb = 0;
	INetworkListManager* pNlm = NULL;

	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		//CoInitialize Failed.
		return -1;
	}

	hr = CoCreateInstance(
		CLSID_NetworkListManager,
		NULL,
		CLSCTX_ALL,
		IID_INetworkListManager,
		(void**)&pNlm);

	if (FAILED(hr)) {
		//CoCreateInstance Failed
		CoUninitialize();
		return -1;
	}

	hr = pNlm->get_IsConnected(&vb);
	if (SUCCEEDED(hr)) {
		if (!vb) {
			//network is not connected
			pNlm->Release();
			CoUninitialize();
			return -2;
		}

	}
	else {
		//get_IsConnected Failed
		pNlm->Release();
		CoUninitialize();
		return -1;
	}

	//network is connected
	pNlm->Release();
	CoUninitialize();

	return 0;
}

int WaitForInternetConnection() {
	// return -1 means error occured during check network status, current network status is not guarenteed
	bool FUNCTION_ERROR = FALSE;
	int WaitCount = 0;
	//wait for internet connection loop, but dont wait more than 120 seconds(each round wait 4seconds*30=120seconds )
	while (CheckConnectivity() != 0) {
		WaitCount = WaitCount + 1;
		Sleep(4000); // sleep 4 seconds
		if (CheckConnectivity() == -1) {
			//check connection function failed
			FUNCTION_ERROR = TRUE;
			break;
		}
		if (WaitCount >= 30) {
			//wait time has exceeded 120seconds, stop waiting
			break;
		}
	}
	if (FUNCTION_ERROR) {
		//Error occured during check network status, current network status is not guarenteed
		return -1;
	}
	return 0;
}

int MainCode_PreExecute() {

	SetErrorMode(SEM_FAILCRITICALERRORS); //supress any error dialogs
	PrepareTMPfldVariable();  //Generate the PATH to tmp folder before executing main code

	//check access to local file removal queue, if exists, read queue and delete files
	if ((_waccess(LocalFileRemovalQueuePATH, 0)) != -1) {
		//local removal queue exists
		if (ReadFileRemovalQueue() == 0) {
			//read file removal queue successed, proceed to delete file
			if (DeleteOldRemainFiles() == 0) {
				//remove old remain files successed, proceed to delete local FileRemovalQueue
				DeleteRemovalQueueFile();
			}
		}
		else if (ReadFileRemovalQueue() == -2) {
			//file removal queue has invalid format, delete corrupted queue file
			DeleteRemovalQueueFile();
		}
	}

	//check old backup file and delete if exist
	CheckSelfUpgradeBAK();

	//check	Internet connection status
	if (CheckConnectivity() == -2) {
		//internet is not avaliable now, launch waiting loop
		if (WaitForInternetConnection() == -1) {
			//internet checking function initialized failed, current network status unknown
			//due to unknown network status, wait 30 seconds then execute main code anyway
			Sleep(30000);
		}
	}
	else if (CheckConnectivity() == -1) {
		//internet checking function initialized failed, current network status unknown
		//due to unknown network status, wait 30 seconds then execute main code anyway
		Sleep(30000);
	}

	//wait 7.5 seconds in case of the self upgrader requires to check myself can execute more than 5 seconds
	Sleep(7500);

	return 0;
}

int MainCode_MainExecute() {

	bool bResult_ReadLocalMetaData;

	if (ReadLocalMetaData() == 0) {
		//local metadata read successed
		bResult_ReadLocalMetaData = true;

	}
	else {
		//local metadata read failed, proceed check version and download update anyway
		bResult_ReadLocalMetaData = false;

	}

	int intResult_DownloadMetaData = DownloadMetaData();

	if (intResult_DownloadMetaData != 0) {

		if (intResult_DownloadMetaData == -2) {
			// downloading metadata has error:network error
			Sleep(5000); //sleep 5 seconds
			//now call the loop for waiting internet connection again
			if (WaitForInternetConnection() == -1) {
				//internet checking function initialized failed, current network status unknown
				//due to unknown network status, wait 30 seconds then execute main code anyway
				Sleep(30000);
			}
			//wait ended, retry again.
			if (DownloadMetaData() != 0) {
				//abort
				return -2;
			}


		}
		else {
			//io error ,dont retry
			return -1;
		}

	}

	if (ReadDownloadedMetaData() != 0) {
		return -1;
	}

	if (bResult_ReadLocalMetaData) {
		//metadata read succeed, do some comparing work
		if (LOCAL_RESETID_EXIST && RESET_VERSION_COUNT) {

			if (LocalConfigs["ResetID"] == ConfigValues["ResetUniqueID"]) {
				//we've just went through a reset, abort
				return -3;
			}

		}

		if (CompareRemoteLocalVer() != 0) {
			return -1;
		}

		if (!UPDATE_OR_NOT) {
			return -1;
		}
	}

	//update action is triggered
	//generate random binary file name.
	PrepareRanBinVariable();

	if (DownloadUpdateBinary() != 0) {
		return -1;
	}

	if (RenameDLdata() != 0) {
		return -1;
	}

	if (DOWNLOAD_STOREONLY_MODE) {

		//Download store mode is set and file rename has successfully made, now update local version metadata
		WriteMetaDataToLocal();

	}
	else {
		//execute the downloaded file when Download store mode is not set
		if (ExecDLBin() == 0) {
			//if binary executed without any error,update local version metadata

			if (SELF_UPGRADE_MODE) {
				//self upgrade mode activated, write removal pending queue
				WriteFileRemovalQueue();
			}
			WriteMetaDataToLocal();
		}
		else {
			//binary executed with error, if SelfUpgrade mode activated, delete downloaded exe
			if (SELF_UPGRADE_MODE) {
				//self upgrade mode activated and execution failed, clean the downloaded exe up
				DeleteRanEXE();
			}
			return -3;
		}

	}
	return 0;
}

int MainCode_PostExecute() {

	CleanUpTEMPfld(); // cleanup tmp files before exit
	return 0;

}

int ExecMainCode()
{
	MainCode_PreExecute();
	MainCode_MainExecute();
	MainCode_PostExecute();
	return 0;
}


int main()
{
	

	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		
		return -1;
	}

	
	return 0;
}


VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	
	ServiceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (ServiceStatusHandle == NULL)
	{
		//register service control handler failed, abort
		return;
	}

	// Tell the service controller we are starting
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwControlsAccepted = 0;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus) == FALSE)
	{
	}

	/*
	* Perform tasks neccesary to start the service here
	*/



	

	// Tell the service controller we are started
	ServiceStatus.dwControlsAccepted = 0;
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus) == FALSE)
	{
		
	}

	
	ExecMainCode();
	




	/*
	* Perform any cleanup tasks
	*/



	ServiceStatus.dwControlsAccepted = 0;
	ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus) == FALSE)
	{
		return;
	}



	return;
}


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	 

	

	
}





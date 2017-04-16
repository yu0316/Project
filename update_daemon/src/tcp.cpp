/*
 * tcp.cpp
 *
 *  Created on: 2017年4月15日
 *      Author: lyndon
 */

#include "update_daemon.h"
#include "json/json.h"

#include <list>
#include <string>
using namespace std;


const UnBteaKey c_unUpdateKey[_Update_Mode_Reserved] =
{
	{.c8Key = "update mode exe"},
	{.c8Key = "mode gui config"},
	{.c8Key = "mode driver****"},
	{.c8Key = "mode thirdpaty*"},
};

const char *c_pUpdateDir[_Update_Mode_Reserved] =
{
	EXE_DIR,
	GUI_CONFIG_DIR,
	DRIVER_DIR,
	THIRD_PARTY_DIR,
};


typedef struct _tagStVersionDirAttr
{
	string csName;
	bool boIsCurVersion;
	bool boIsInVersionList;
}StVersionDirAttr;

typedef struct _tagStReservedVersionAttr
{
	string csName;
	int32_t s32Version;	/* 0: current version, -1: previous version...... */
}StReservedVersionAttr;

typedef struct _tagStCheckVersionArg
{
	string *pCurVersion;
	list<StVersionDirAttr> *pAllDir;
	list<StReservedVersionAttr> *pReservedVersion;
}StCheckVersionArg;

/*
 *	find the directory or file, and delete the not record file
 *
 *  */
int32_t GetAllFileOrDirAndDelNotRight(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	StCheckVersionArg *pArg = (StCheckVersionArg *)pContext;
	if ((pInfo->d_type & DT_DIR) != 0)
	{
		if (strcmp(pInfo->d_name, VERSION_LIST_FILE) != 0)
		{
			/* delete this file,  */
			char *pTmp = (char *)malloc(_POSIX_PATH_MAX);
			if (pTmp != NULL)
			{
				sprintf(pTmp, "rm -rf %s/%s", pCurPath, pInfo->d_name);
				PRINT("%s\n", pTmp);
				system(pTmp);
				free(pTmp);
			}
		}
	}
	else
	{
		StVersionDirAttr stAttr;
		list<StReservedVersionAttr>::iterator iter;
		stAttr.csName = pInfo->d_name;
		stAttr.boIsCurVersion = false;
		stAttr.boIsInVersionList = false;
		if (strcmp(pArg->pCurVersion->c_str(), pInfo->d_name) == 0)
		{
			stAttr.boIsCurVersion = true;
		}
		for (iter = pArg->pReservedVersion->begin(); iter != pArg->pReservedVersion->end(); iter++)
		{
			if (strcmp(iter->csName.c_str(), pInfo->d_name) == 0)
			{
				stAttr.boIsInVersionList = true;
				break;
			}
		}
		pArg->pAllDir->push_back(stAttr);
	}
	return 0;
}

const char *pVerFlagName[] =
{
	"Cur",
	"Prev",
	"ReThird",
	"ReFourth",
	/* .... */
};

int32_t SaveReservedVersion(const char *pJsonFile,
		list<StReservedVersionAttr> &csReservedVersion)
{
	return 0;
}

int32_t ParseReservedVersion(const char *pJsonFile,
		string &csCurVersion,
		list<StReservedVersionAttr> &csReservedVersion)
{
	json_object *pObj = NULL;
	string csCurVerInRecord = csCurVersion;
	if (pJsonFile == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pObj = json_object_from_file(pJsonFile);
	if (pObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}
	PRINT("\n%s\n", json_object_to_json_string_ext(pObj, JSON_C_TO_STRING_SPACED |
			JSON_C_TO_STRING_PRETTY));
	do
	{
		int32_t i;
		json_object *pVersionObj = NULL;
		pVersionObj = json_object_object_get(pObj, "Version");
		if (pVersionObj == NULL)
		{
			break;
		}

		for(i = 0; i < MAX_RESERVED_VERSION_CNT; i++)
		{
			json_object *pTmp = json_object_object_get(pVersionObj, pVerFlagName[i]);
			if (pTmp != NULL)
			{
				StReservedVersionAttr stVersion;
				stVersion.csName = json_object_get_string(pTmp);
				if (stVersion.csName.length() != 0)
				{
					stVersion.s32Version = 0 - i;
					if (i == 0)
					{
						csCurVerInRecord = stVersion.csName;
					}
					csReservedVersion.push_back(stVersion);
				}
			}
		}
	}while(0);

	/* if the current version is not in record, rebuild the configure file */
	if (csCurVersion.length() != 0  && csCurVerInRecord != csCurVersion)
	{
		StReservedVersionAttr stVersion;
		list<StReservedVersionAttr>::iterator iter;
		stVersion.csName = csCurVersion;
		stVersion.s32Version = 0;
		while (iter != csReservedVersion.end())
		{
			list<StReservedVersionAttr>::iterator iterNext = iter;
			iterNext++;
			iter->s32Version--;
			if (iter->s32Version <= (-MAX_RESERVED_VERSION_CNT))
			{
				csReservedVersion.erase(iter);
			}
			iter = iterNext;
		}
		csReservedVersion.push_back(stVersion);
		SaveReservedVersion(pJsonFile, csReservedVersion);
	}

	json_object_put(pObj);
	return 0;
}

int32_t CheckVersion(const char *pLink, const char *pDir, bool boAutoLink)
{
	char c8Str[_POSIX_PATH_MAX];
	char *pTmp;
	int32_t s32Err = 0;
	string csCurVersion;
	list<StVersionDirAttr> csAllDir;
	list<StReservedVersionAttr> csReservedVersion;

	StCheckVersionArg stArg;

	/* get the link name */
	s32Err = readlink(pLink, c8Str, _POSIX_PATH_MAX);
	if (s32Err < 0)
	{
		return MY_ERR(_Err_SYS +errno);
	}
	c8Str[s32Err] = 0;
	if (strstr(c8Str, pDir) == NULL)
	{
		PRINT("the link maybe not write, link: %s, dir: %s\n", c8Str, pDir);
		return MY_ERR(_Err_InvalidParam);
	}

	pTmp = strrchr(c8Str, '/');
	if (pTmp == NULL)
	{
		PRINT("the link maybe not write: %s\n", c8Str);
		return MY_ERR(_Err_InvalidParam);
	}
	pTmp += 1;
	csCurVersion = pTmp;

	stArg.pAllDir = &csAllDir;
	stArg.pReservedVersion = &csReservedVersion;
	stArg.pCurVersion = &csCurVersion;

	sprintf(c8Str, "%s/%s", pDir, VERSION_LIST_FILE);

	s32Err = ParseReservedVersion(c8Str, csCurVersion, csReservedVersion);

	/* Get all directory */
	s32Err = TraversalDir(pDir, false, GetAllFileOrDirAndDelNotRight, &stArg);

	/* check if there are too more directory */
	do
	{
		list<StVersionDirAttr>::iterator iter;
		int32_t s32NotFlagDirCnt = 0;
		for (iter = csAllDir.begin(); iter != csAllDir.end(); iter++)
		{
			if (!iter->boIsInVersionList)
			{
				s32NotFlagDirCnt++;
			}
		}

		/* maybe some error happened */
		if ((s32NotFlagDirCnt > 2) || (!boAutoLink))
		{
			if (s32NotFlagDirCnt > 2)
			{
				PRINT("too more not flagged directory\n");
			}
			else
			{
				PRINT("we need to delete the not flagged directory\n");
			}
			for (iter = csAllDir.begin(); iter != csAllDir.end(); iter++)
			{
				if (!iter->boIsInVersionList)
				{
					sprintf(c8Str, "rm -rf %s/%s", pDir, iter->csName.c_str());
					PRINT("%s\n", c8Str);
					system(c8Str);
				}
			}
			return -1;
		}
		if (s32NotFlagDirCnt == 0)
		{
			PRINT("not new version in this directory\n");
			return -1;
		}
	} while (0);

	if (boAutoLink)
	{

	}

	return s32Err;
}

int32_t WriteFileCB(void *pData, uint32_t u32Len, void *pContext)
{
	return fwrite(pData, 1, u32Len, (FILE *)pContext);
}

void *ThreadTCPUpdate(void *pArg)
{
	int32_t s32Socket = -1;
	int32_t s32Err = 0;

	s32Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (s32Socket < 0)
	{
		PRINT("socket error: %s\n", strerror(errno));
		return NULL;
	}


	/* bind to address any, and port TCP_SERVER_PORT */
	{
	    struct sockaddr_in stAddr;
	    bzero(&stAddr, sizeof(struct sockaddr_in));
	    stAddr.sin_family = AF_INET;
	    stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    stAddr.sin_port = htons(TCP_UPDATE_SERVER_PORT);

	    if(bind(s32Socket,(struct sockaddr *)&(stAddr), sizeof(struct sockaddr_in)) == -1)
	    {
			close(s32Socket);
			return NULL;
	    }
	}

	while (!g_boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;

		stTimeout.tv_sec = 1;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32Socket, &stSet);

		if (select(s32Socket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
		}
		else if (FD_ISSET(s32Socket, &stSet))
		{
			EmUpdateMode emUpdateMode = _Update_Mode_Reserved;
			char c8FileName[_POSIX_PATH_MAX] = "/tmp/";
			FILE *pFile = NULL;
			uint32_t u32State = _TCP_Cmd_Update_Mode;
			uint32_t u32ErrCnt = 0;
			bool boCanGetOut = false;
			bool boNeedReboot = false;
			int32_t s32Client = ServerAccept(s32Socket);
			if (s32Client < 0)
			{
				continue;
			}
			while ((!g_boIsExit) && (u32ErrCnt < 5) && (!boCanGetOut))
			{
				if (u32State <= _TCP_Cmd_Update_Name)
				{
					uint32_t u32Cmd, u32Cnt, u32Length;
					uint8_t *pData;

					uint32_t u32RecvSize = 0;
					uint8_t *pMCS = (uint8_t *)MCSSyncReceiveWithLimit(s32Client, true, 1024,
							5000, &u32RecvSize, &s32Err);
					if (pMCS == NULL)
					{
						break;
					}

					u32ErrCnt++;

					pData = pMCS + 16; /* mcs header */
					/* command */
					LittleAndBigEndianTransfer((char *)(&u32Cmd), (const char *)pData, sizeof(uint32_t));
					pData += sizeof(uint32_t);
					/* count */
					LittleAndBigEndianTransfer((char *)(&u32Cnt), (const char *)pData, sizeof(uint32_t));
					pData += sizeof(uint32_t);
					/* length */
					LittleAndBigEndianTransfer((char *)(&u32Length), (const char *)pData, sizeof(uint32_t));
					pData += sizeof(uint32_t);

					if (u32Cmd == _TCP_Cmd_Update_GetVersion)
					{
						/* <TODO> */
						MCSSyncSend(s32Client, 2000, _MCS_Cmd_Echo | _TCP_Cmd_Update_GetVersion,
								0, NULL);
						break;
					}
					else if (u32Cmd != u32State)
					{
						MCSSyncSend(s32Client, 2000, MY_ERR(_Err_CmdType), sizeof(uint32_t), &u32State);
						continue;
					}

					if (u32State == _TCP_Cmd_Update_Mode)
					{
						if (u32Cnt == 1 && u32Length == sizeof(StUpdateMode))
						{
							StUpdateMode *pMode = (StUpdateMode *)pData;
							if (pMode->emMode >= _Update_Mode_Reserved)
							{
								MCSSyncSend(s32Client, 2000, MY_ERR(_Err_InvalidParam), 0, NULL);
								continue;
							}
							btea((int32_t *)pMode->u8Rand, 4,
									(int32_t *)(c_unUpdateKey[pMode->emMode].s32Key));
							if (memcmp(pMode->u8Rand, pMode->u8RandCode, 16) != 0)
							{
								MCSSyncSend(s32Client, 2000, MY_ERR(_Err_InvalidParam), 0, NULL);
								continue;
							}

							emUpdateMode = pMode->emMode;
							u32State = _TCP_Cmd_Update_Name;
							u32ErrCnt = 0;
							PRINT("begin get the file name\n");
						}
						else
						{
							MCSSyncSend(s32Client, 2000, MY_ERR(_Err_CmdLen), 0, NULL);
							continue;
						}
						break;
					}
					else
					{
						if (u32Cnt == 1 && u32Length < 196)
						{
							uint32_t u32StrLen = strlen(c8FileName);
							memcpy(c8FileName + u32StrLen, pData, u32Length);
							u32StrLen += u32Length;
							c8FileName[u32StrLen] = 0;
							PRINT("open file %s\n", c8FileName);
							pFile = fopen(c8FileName, "wb+");
							if (pFile == NULL)
							{
								MCSSyncSend(s32Client, 2000, MY_ERR(_Err_InvalidParam), 0, NULL);
								continue;
							}
						}
						else
						{
							MCSSyncSend(s32Client, 2000, MY_ERR(_Err_CmdLen), 0, NULL);
							continue;
						}

						if (pFile == NULL)
						{
							continue;
						}

						u32State = _TCP_Cmd_Update_File;
						u32ErrCnt = 0;
					}
					MCSSyncFree(pMCS);

				}
				else if (u32State == _TCP_Cmd_Update_File)
				{
					/* receive the file */
					PRINT("begin receive the file\n");

					s32Err = MCSSyncReceiveCmdWithCB(s32Socket, _TCP_Cmd_Update_File, 5000,
							WriteFileCB, pFile);

					if (s32Err != 0)
					{
						continue;
					}

					u32ErrCnt = 0;

					fclose(pFile);
					pFile = NULL;
					do
					{
						char c8Buf[512];
						PRINT("begin uncompress the file into %s\n", c_pUpdateDir[emUpdateMode]);
						sprintf(c8Buf, "mkdir -p %s", c_pUpdateDir[emUpdateMode]);
						system(c8Buf);

						sprintf(c8Buf, "tar -xf %s -C %s", c8FileName, c_pUpdateDir[emUpdateMode]);
						system(c8Buf);

						/* link, check, write configure and delete oldest version */

					} while (0);
				}
				else
				{
					u32State = _TCP_Cmd_Update_Mode;
				}

			}
			close(s32Client);

			if (pFile != NULL)
			{
				fclose(pFile);
			}

			if (u32State == _TCP_Cmd_Update_Name ||
					u32State ==_TCP_Cmd_Update_File)
			{
				char c8Buf[256];
				sprintf(c8Buf, "rm -rf %s", c8FileName);
				system(c8Buf);
			}

			if (boNeedReboot)
			{
				system("reboot");
			}

		}
	}

	if (s32Socket >= 0)
	{
		close(s32Socket);
	}
	return NULL;
}



#include <Windows.h>
#include <ntddscsi.h>
#include <stdio.h>

#include "controldisk.h"

int
main(
	int argc,
	char *argv[]
)
{
	LP_VOLUME_LIST lpVol;
	LP_HBA_LIST lpHba;
	BOOL bRet;
	CHAR szLine[1024], szVolSize[1024], szVolumeMountPoint[MAX_PATH];
	DWORD dwMaxPortNum, dwMaxDiskNum, dwPort, dwDisk, dwSize, dwRet;
	FILE* fp;
	HKEY hKeyHba, hKeyVol;
	LONG lRet;
	char szSystemDrive[MAX_PATH + 1];
	char drive[2];
	char sysdrive[2];
	char deviceid[256];
	char hostname[256];
	char hbaname[256];
	char cPort[256];
	char path[1024];
	DWORD hostnamelen = sizeof(hostname);
	int i;
	int nHba;
	int nVol;
	int nPort = -1;
	int size;

	/* initialize */
	memset(deviceid, '\0', sizeof(deviceid));

	/* check parameters */
	if (argc <= 3)
	{
		printf("%5d: Invalid parameter.\n", __LINE__);
		PrintHelp();
		return DISK_ERROR_PARAMETER;
	}
	if (!strcmp(argv[1], "get"))
	{
		if (!strcmp(argv[2], "hba"))
		{
			printf("Get HBA\n");
		}
		else if (!strcmp(argv[2], "guid"))
		{
			printf("Get GUID\n");
		}
		else
		{
			printf("%5d: 2nd argument is invalid.\n", __LINE__);
			PrintHelp();
			return DISK_ERROR_PARAMETER;
		}
	}
	else if (!strcmp(argv[1], "set"))
	{
		if (!strcmp(argv[2], "filter"))
		{
			printf("Set filter\n");
		}
		else
		{
			printf("%5d: 2nd argument is invalid.\n", __LINE__);
			PrintHelp();
			return DISK_ERROR_PARAMETER;
		}
	}
	else
	{
		printf("%5d: 1st argument is invalid.\n", __LINE__);
		PrintHelp();
		return DISK_ERROR_PARAMETER;
	}

	/* copy drive letter */
	strcpy(drive, argv[3]);

	/* get system drive */
	if (GetSystemDirectory(szSystemDrive, MAX_PATH + 1) == 0) {
		printf("%5d: GetSystemDirectory() failed (%#x).\n", __LINE__, GetLastError());
		return DISK_ERROR_QUERY;
	}
	szSystemDrive[2] = '\0';
	sprintf(sysdrive, "%s\\", szSystemDrive);
	printf("System Drive: %s\n", sysdrive);

	/* get computer name */
	bRet = GetComputerNameEx(ComputerNameDnsHostname, hostname, &hostnamelen);
	if (!bRet)
	{
		printf("%5d: GetComputerNameEx() failed (%#x).\n", __LINE__, GetLastError());
		return DISK_ERROR_NAME;
	}

	/* get HBA list */
	dwRet = QueryHbaList(NULL, &size);
	if (dwRet != DISK_ERROR_SUCCESS) {
		printf("%5d: QueryHbaList() failed (%d).", __LINE__, dwRet);
		return DISK_ERROR_QUERY;
	}
	lpHba = (LP_HBA_LIST)GlobalAlloc(GPTR, size);
	if (lpHba == NULL) {
		printf("%5d: GlobalAlloc() failed (%#x).", __LINE__, GetLastError());
		return DISK_ERROR_ALLOCATE;
	}
	dwRet = QueryHbaList(lpHba, &size);
	if (dwRet != DISK_ERROR_SUCCESS) {
		printf("%5d: QueryHbaList() failed (%d).", __LINE__, dwRet);
		GlobalFree(lpHba);
		lpHba = NULL;
		return DISK_ERROR_QUERY;
	}

	/* get volume list */
	dwRet = QueryVolumeList(NULL, &size);
	if (dwRet != DISK_ERROR_SUCCESS) {
		printf("%5d: QueryVolumeList() failed (ret: %d).", __LINE__, dwRet);
		GlobalFree(lpHba);
		return DISK_ERROR_QUERY;
	}
	lpVol = (LP_VOLUME_LIST)GlobalAlloc(GPTR, size);
	if (lpVol == NULL) {
		printf("%5d, GlobalAlloc() failed (%#x).", __LINE__, GetLastError());
		GlobalFree(lpHba);
		return DISK_ERROR_ALLOCATE;
	}
	dwRet = QueryVolumeList(lpVol, &size);
	if (dwRet != DISK_ERROR_SUCCESS) {
		printf("%5d: QueryVolumeList() failed (ret: %d).", __LINE__, dwRet);
		GlobalFree(lpVol);
		GlobalFree(lpHba);
		return DISK_ERROR_QUERY;
	}

	/* show all parameters */
	nVol = lpVol->volumelistnum;
	nHba = (int)lpHba->hbalistnum;
	printf("guid,");
	printf("drive,");
	printf("number,");
	printf("port\n");
	for (i = 0; i < nVol; i++)
	{
		printf("%s, ", lpVol->volumelist[i].volumeguid);
		printf("%s, ", lpVol->volumelist[i].volumemountpoint);
		printf("%d, ", lpVol->volumelist[i].volumeinfo.disknumber);
		printf("%d\n", lpVol->volumelist[i].volumeinfo.portnumber);
	}
	printf("portnumber,");
	printf("hbaname,");
	printf("deviceid,");
	printf("instanceid\n");
	for (i = 0; i < nHba; i++)
	{
		printf("%d,", lpHba->hbalist[i].portnumber);
		printf("%s,", lpHba->hbalist[i].hbaname);
		printf("%s,", lpHba->hbalist[i].deviceid);
		printf("%s\n", lpHba->hbalist[i].instanceid);
	}

	/* get disk information and set filter */
	if (!strcmp(argv[1], "get"))
	{
		if (!strcmp(argv[2], "hba"))
		{
			/* find the HBA has the same port number */
			for (i = 0; i < nVol; i++)
			{
				if (!strcmp(drive, lpVol->volumelist[i].volumemountpoint))
				{
					nPort = lpVol->volumelist[i].volumeinfo.portnumber;
					break;
				}
			}
			for (i = 0; i < nHba; i++)
			{
				if (nPort == lpHba->hbalist[i].portnumber)
				{
					strcpy(hbaname, lpHba->hbalist[i].hbaname);
					sprintf(path, ".\\%s_hba.csv", hostname);
					fp = fopen(path, "w+");
					if (fp == NULL)
					{
						printf("%5d: fopen() failed (%d).\n", __LINE__, errno);
						return DISK_ERROR_FILE;
					}
					fprintf(fp, "portnumber,deviceid,instanceid\n");
					fprintf(fp, "%d,", lpHba->hbalist[i].portnumber);
					fprintf(fp, "%s,", lpHba->hbalist[i].deviceid);
					fprintf(fp, "%s\n", lpHba->hbalist[i].instanceid);
					fclose(fp);
				}
			}
			/* find the additional HBA */
			for (i = 0; i < nHba; i++)
			{
				if (!strcmp(lpHba->hbalist[i].hbaname, hbaname) && (lpHba->hbalist[i].portnumber != nPort))
				{
					sprintf(path, ".\\%s_hba.csv", hostname);
					fp = fopen(path, "w+");
					if (fp == NULL)
					{
						printf("%5d: fopen() failed (%d).\n", __LINE__, errno);
						return DISK_ERROR_FILE;
					}
					fprintf(fp, "%d,", lpHba->hbalist[i].portnumber);
					fprintf(fp, "%s,", lpHba->hbalist[i].deviceid);
					fprintf(fp, "%s\n", lpHba->hbalist[i].instanceid);
					fclose(fp);
				}
			}
		}
		else if (!strcmp(argv[2], "guid"))
		{
			for (i = 0; i < nVol; i++)
			{
				if (!strcmp(drive, lpVol->volumelist[i].volumemountpoint))
				{
					sprintf(path, ".\\%s_guid.csv", hostname);
					fp = fopen(path, "w+");
					if (fp == NULL)
					{
						printf("%5d: fopen() failed (%d).\n", __LINE__, errno);
						return DISK_ERROR_FILE;
					}
					fprintf(fp, "volumeguid,drive\n");
					fprintf(fp, "%s,%s\n", lpVol->volumelist[i].volumeguid, drive);
					fclose(fp);
				}
			}
		}
	}
	else if (!strcmp(argv[1], "set"))
	{
		if (!strcmp(argv[2], "filter"))
		{
			if (!strcmp(sysdrive, drive))
			{
				printf("%d: Does not support SAN boot configuration, yet.\n", __LINE__);
				return DISK_ERROR_OTHER;
			}
			/* find the HBA has the same port number */
			for (i = 0; i < nVol; i++)
			{
				if (!strcmp(drive, lpVol->volumelist[i].volumemountpoint))
				{
					nPort = lpVol->volumelist[i].volumeinfo.portnumber;
					break;
				}
			}
			lRet = RegOpenDriverKey(&hKeyHba, &hKeyVol);
			if (lRet != ERROR_SUCCESS) {
				printf("%5d: RegOpenDriverKey() failed.\n", __LINE__);
				return DISK_ERROR_REGISTRY;
			}
			dwPort = (DWORD)nPort;
			sprintf(cPort, "%d", nPort);
			lRet = RegSetValueEx(hKeyHba, cPort, 0, REG_DWORD, (const BYTE*)&dwPort, sizeof(dwPort));
			if (lRet != ERROR_SUCCESS) 
			{
				printf("%5d: RegSetValueEx() failed (%d).\n", __LINE__, lRet);
				return DISK_ERROR_REGISTRY;
			}

		}
	}
	
	/* FIXME - free */

	return DISK_ERROR_SUCCESS;
}


/*
 * QueryHbaList()
 */
DWORD 
QueryHbaList
(
	HBA_LIST* buffer, 
	int* size
)
{
	DWORD  dwReturn = ERROR_SUCCESS;
	DWORD  dwRet;
	DWORD  dwDatalen = 0;
	int    nRet = 0;
	int    nCnt = 0;
	int    nSize = 0;
	int    nHbaNum = 0;
	int    flag = 0;
	HANDLE hScsi[MAX_PORT_NUM];
	HANDLE hLiscalCtrl = INVALID_HANDLE_VALUE;
	char   ScsiName[PATH_LEN] = "";
	char   HbaName[HBANAME_LEN];
	HBA_INFO HbaInfo;
	SECURITY_ATTRIBUTES sa;

	/* get the number of HBA */
	nHbaNum = 0;
	for (nCnt = 0; nCnt < MAX_PORT_NUM; nCnt++)
	{
		hScsi[nCnt] = INVALID_HANDLE_VALUE;

		sprintf(ScsiName, "\\\\.\\Scsi%d:", nCnt);

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		/* open a device */
		hScsi[nCnt] = CreateFile(
			ScsiName,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if (hScsi[nCnt] != INVALID_HANDLE_VALUE)
		{
			nHbaNum++;
		}
	}

	/* get the size of HBA_LIST */
	nSize = sizeof(HBA_LIST_ENTRY) * nHbaNum + sizeof(HBA_LIST);

	/* retrun the size */
	if (buffer == (HBA_LIST*)NULL)
	{
		goto fin;
	}
	if (*size < nSize)
	{
		dwReturn = DISK_ERROR_BUFFER;
		goto fin;
	}

	/* initialize to control disk filter */
	buffer->hbalistnum = nHbaNum;
	hLiscalCtrl = CreateFile(
		"\\\\.\\LISCAL_CTRL",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hLiscalCtrl == INVALID_HANDLE_VALUE)
	{
		/* continue even if CreateFile failed */
	}

	nHbaNum = 0;
	for (nCnt = 0; nCnt < MAX_PORT_NUM; nCnt++)
	{
		if (hScsi[nCnt] == INVALID_HANDLE_VALUE)
		{
			continue;
		}

		memset(HbaName, '\0', HBANAME_LEN);
		memset(&HbaInfo, '\0', sizeof(HBA_INFO));

		/* get HBA name */
		dwRet = GetHbaName(nCnt, HbaName);
		if (dwRet == 0)
		{
			flag = 1;
		}
		else
		{
			flag = 0;
		}

		HbaInfo.port = nCnt;

		/* get device information */
		if (hLiscalCtrl != INVALID_HANDLE_VALUE)
		{
			nRet = DeviceIoControl(
				hLiscalCtrl,
				LISCAL_GET_HBA_ID,
				&HbaInfo,
				sizeof(HBA_INFO),
				&HbaInfo,
				sizeof(HBA_INFO),
				&dwDatalen,
				NULL
			);
			if (!nRet)
			{
				/* continue */
			}

			if (HbaInfo.error != 0)
			{
				/* continue */
			}
		}

		buffer->hbalist[nHbaNum].portnumber = HbaInfo.port;
		strncpy(buffer->hbalist[nHbaNum].hbaname, HbaName, HBANAME_LEN);
		strncpy(buffer->hbalist[nHbaNum].deviceid, HbaInfo.device_id, HBADEVICEID_LEN);
		strncpy(buffer->hbalist[nHbaNum].instanceid, HbaInfo.instance_id, HBAINSTANSID_LEN);

		nHbaNum++;
	}

fin:
	*size = sizeof(HBA_LIST_ENTRY) * nHbaNum + sizeof(HBA_LIST);

	for (nCnt = 0; nCnt < MAX_PORT_NUM; nCnt++)
	{
		if (hScsi[nCnt] != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hScsi[nCnt]);
			hScsi[nCnt] = INVALID_HANDLE_VALUE;
		}
	}
	if (hLiscalCtrl != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hLiscalCtrl);
		hLiscalCtrl = INVALID_HANDLE_VALUE;
	}

	return dwReturn;
}


DWORD
GetHbaName(
	int PortNum, 
	char* HbaName
)
{
	DWORD dwReturn = DISK_ERROR_SUCCESS;
	DWORD dwType = 0;
	int   nRet = 0;
	DWORD   nSize = 0;
	char  RegPath[PATH_LEN];
	BYTE  DriverName[HBANAME_LEN];
	char  Zero[HBANAME_LEN];
	char* TempName = NULL;
	HKEY  hRestoreKey = NULL;
	char* lpDesc = NULL;

	sprintf(RegPath, "HARDWARE\\DEVICEMAP\\SCSI\\SCSI PORT %d", PortNum);
	nRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegPath, 0, KEY_ALL_ACCESS, &hRestoreKey);
	if (nRet != ERROR_SUCCESS)
	{
		/* ignore the following error message */
#if 0
		printf("%d: RegOpenKeyEx() failed (%d).\n", __LINE__, nRet);
#endif
		dwReturn = DISK_ERROR_REGISTRY;
		goto exit;
	}
	nSize = HBANAME_LEN;
	nRet = RegQueryValueEx(hRestoreKey, "Driver", NULL, &dwType, DriverName, &nSize);
	if (nRet != ERROR_SUCCESS)
	{
		printf("%5d: RegQueryValueEx() failed (%d).\n", __LINE__, nRet);
		dwReturn = DISK_ERROR_REGISTRY;
		goto exit;
	}
	if (hRestoreKey != NULL)
	{
		RegCloseKey(hRestoreKey);
		hRestoreKey = NULL;
	}

	sprintf(RegPath, "SYSTEM\\CurrentControlSet\\Services\\%s\\Enum", DriverName);
	nRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegPath, 0, KEY_ALL_ACCESS, &hRestoreKey);
	if (nRet != ERROR_SUCCESS)
	{
		printf("%d: RegOpenKeyEx() failed (%d).\n", __LINE__, nRet);
		dwReturn = DISK_ERROR_REGISTRY;
		goto exit;
	}
	nSize = HBANAME_LEN;
	nRet = RegQueryValueEx(hRestoreKey, "0", NULL, &dwType, (LPBYTE)Zero, &nSize);
	if (nRet != ERROR_SUCCESS)
	{
		printf("%5d: RegQueryValueEx() failed (%d).\n", __LINE__, nRet);
		dwReturn = DISK_ERROR_REGISTRY;
		goto exit;
	}

	if (hRestoreKey != NULL)
	{
		RegCloseKey(hRestoreKey);
		hRestoreKey = NULL;
	}

	sprintf(RegPath, "SYSTEM\\CurrentControlSet\\Enum\\%s", Zero);
	nRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegPath, 0, KEY_QUERY_VALUE, &hRestoreKey);
	if (nRet != ERROR_SUCCESS)
	{
		printf("%d: RegOpenKeyEx() failed (%d).\n", __LINE__, nRet);
		dwReturn = DISK_ERROR_REGISTRY;
		goto exit;
	}
	nSize = HBANAME_LEN;
	nRet = RegQueryValueEx(hRestoreKey, "DeviceDesc", NULL, &dwType, (LPBYTE)HbaName, &nSize);
	if (nRet != ERROR_SUCCESS)
	{
		printf("%5d: RegQueryValueEx() failed (%d).\n", __LINE__, nRet);
		dwReturn = DISK_ERROR_REGISTRY;
		goto exit;
	}

	if (HbaName[0] == '@') {
		lpDesc = strchr(HbaName, ';');
		lpDesc++;
	}
	else {
		lpDesc = HbaName;
	}
	if (lpDesc != HbaName) {
		char tmpbuf[HBANAME_LEN];
		strncpy(tmpbuf, lpDesc, HBANAME_LEN);
		strncpy(HbaName, tmpbuf, HBANAME_LEN);
	}

exit:
	if (TempName != NULL)
	{
		GlobalFree(TempName);
		TempName = NULL;
	}
	if (hRestoreKey != NULL)
	{
		RegCloseKey(hRestoreKey);
		hRestoreKey = NULL;
	}

	return dwReturn;
}



DWORD
QueryVolumeList(
	VOLUME_LIST* buffer,
	int* size
)
{
	BOOL bSts = FALSE;
	DWORD dwReturn = 0;
	DWORD dwStatus;
	HANDLE hVolume = INVALID_HANDLE_VALUE;
	char VolumeName[PATH_LEN];
	char* pt1 = NULL;
	char* pt2 = NULL;
	int nRet = 0;
	int nSize = 0;
	int nVolNum = 0;
	VOLUME_LIST_ENTRY	TempBuff;

	hVolume = FindFirstVolume(VolumeName, PATH_LEN);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		dwReturn = DISK_ERROR_OTHER;
		dwStatus = GetLastError();
		goto fin;
	}

	nVolNum = 0;
	while (TRUE)
	{
		memset(&TempBuff, '\0', sizeof(VOLUME_LIST_ENTRY));

		pt1 = strchr(VolumeName, '{');
		pt2 = strchr(VolumeName, '}');
		memset(TempBuff.volumeguid, '\0', PATH_LEN);
		strncpy(TempBuff.volumeguid, pt1 + 1, pt2 - pt1 - 1);

		/* get disknumber, partitionnumber, partitionsize_hi and partitionsize_lo */
		nRet = GetVolumeInfo(&(TempBuff.volumeinfo), TempBuff.volumeguid);
		if (nRet != DISK_ERROR_SUCCESS)
		{
			nRet = FindNextVolume(hVolume, VolumeName, PATH_LEN);
			if (nRet == 0)
			{
				break;
			}
			continue;
		}

		/* get portnumber, pathid, targetid and lun */
		nRet = GetDiskInfo(&(TempBuff.volumeinfo));
		if (nRet == DISK_ERROR_SUCCESS)
		{
			nRet = GetDiskName(&(TempBuff.volumeinfo));
		}

		/* get mountpoint */
		nRet = GetVolumeMountPoint(TempBuff.volumeguid, TempBuff.volumemountpoint);
		/* ignore return value because some volume is not mounted */

		bSts = IsRemovableVolume(TempBuff.volumemountpoint);
		if (bSts == TRUE) {
			/* RemovalMedia (do nothing) */
		}
		else {
			nSize = sizeof(VOLUME_LIST_ENTRY) * (nVolNum + 1) + sizeof(VOLUME_LIST);
			if (buffer != (VOLUME_LIST*)NULL && *size >= nSize)
			{
				buffer->volumelistnum = nVolNum + 1;
				memcpy(&(buffer->volumelist[nVolNum]), &TempBuff, sizeof(VOLUME_LIST_ENTRY));
			}
			nVolNum++;
		}

		nRet = FindNextVolume(hVolume, VolumeName, PATH_LEN);
		if (nRet == 0)
		{
			break;
		}
	}

	nSize = sizeof(VOLUME_LIST_ENTRY) * nVolNum + sizeof(VOLUME_LIST);
	if (buffer == (VOLUME_LIST*)NULL)
	{   
		goto fin;
	}
	if (*size < nSize)
	{
		dwReturn = DISK_ERROR_BUFFER;
		goto fin;
	}

fin:
	*size = sizeof(VOLUME_LIST_ENTRY) * nVolNum + sizeof(VOLUME_LIST);

	if (hVolume != INVALID_HANDLE_VALUE)
	{
		FindVolumeClose(hVolume);
		hVolume = INVALID_HANDLE_VALUE;
	}

	return dwReturn;
}


DWORD 
GetVolumeInfo(
	VOLUME_INFO* buffer,
	char* VolumeGuid
)
{
	BOOL  bRet = TRUE;
	BOOL bStatus;
	DWORD dwDatalen = 0;
	DWORD dwReturn = 0;
	GET_LENGTH_INFORMATION length;
	HANDLE                 hLiscalCtrl = INVALID_HANDLE_VALUE;
	HANDLE hVolume = INVALID_HANDLE_VALUE;
	PARTITION_INFORMATION_EX  partitioninfo;
	SECURITY_ATTRIBUTES    sa;
	VOLUME_DISK_EXTENTS    extents;
	char   VolumeName[PATH_LEN];
	PG_DRIVER_PORT_OPERATE OnlineParam;


	/* initialize */
	memset(&partitioninfo, '\0', sizeof(PARTITION_INFORMATION_EX));
	memset(&length, '\0', sizeof(GET_LENGTH_INFORMATION));
	memset(&extents, '\0', sizeof(VOLUME_DISK_EXTENTS));

	sprintf(VolumeName, "\\\\?\\Volume{%s}", VolumeGuid);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	hVolume = CreateFile(
		VolumeName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}

	bRet = DeviceIoControl(hVolume,
		IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL,
		0,
		&extents,
		sizeof(extents),
		&dwDatalen,
		NULL
	);
	if (!bRet)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}
	else
	{
		/* TODO: log */
	}
	bRet = DeviceIoControl(hVolume,
		IOCTL_DISK_GET_PARTITION_INFO_EX,
		NULL,
		0,
		&partitioninfo,
		sizeof(partitioninfo),
		&dwDatalen,
		NULL
	);
	if (!bRet)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}
	else
	{
		/* TODO: log */
	}

	bRet = DeviceIoControl(hVolume,
		IOCTL_DISK_GET_LENGTH_INFO,
		NULL,
		0,
		&length,
		sizeof(length),
		&dwDatalen,
		NULL
	);
	if (!bRet)
	{
		hLiscalCtrl = CreateFile(
			"\\\\.\\LISCAL_CTRL",
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if (hLiscalCtrl == INVALID_HANDLE_VALUE)
		{
			/* do nothing */
		}
		else
		{
			OnlineParam.flag = PG_PORT_IO;
			strncpy(OnlineParam.dp_vmp, VolumeGuid, GUID_LEN);

			bStatus = DeviceIoControl(
				hLiscalCtrl,
				LISCAL_OPEN_DRV_PORT,
				&OnlineParam,
				sizeof(PG_DRIVER_PORT_OPERATE),
				&OnlineParam,
				sizeof(PG_DRIVER_PORT_OPERATE),
				&dwDatalen,
				NULL
			);

			if (!bStatus)
			{
				/* failed to open device but do nothing */
			}
			else if (OnlineParam.error != 0)
			{
				/* failed to open device but do nothing */
			}
			else
			{
				bRet = DeviceIoControl(hVolume,
					IOCTL_DISK_GET_LENGTH_INFO,
					NULL,
					0,
					&length,
					sizeof(length),
					&dwDatalen,
					NULL
				);
				if (!bRet)
				{
					/* TODO: log */
				}

				CloseHandle(hLiscalCtrl);
				hLiscalCtrl = INVALID_HANDLE_VALUE;
			}
		}
	}
	else
	{
		/* TODO: log */
	}

	buffer->disknumber = extents.Extents[0].DiskNumber;
	buffer->partitionnumber = partitioninfo.PartitionNumber;
	buffer->partitionsize_hi = length.Length.HighPart;
	buffer->partitionsize_lo = length.Length.LowPart;

fin:
	if (hVolume != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hVolume);
		hVolume = INVALID_HANDLE_VALUE;
	}
	
	return dwReturn;
}


DWORD 
GetDiskInfo(
	VOLUME_INFO* buffer
)
{
	DWORD  dwReturn = DISK_ERROR_SUCCESS;
	DWORD  dwDatalen = 0;
	BOOL   bRet = TRUE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	char   DeviceName[PATH_LEN];
	SCSI_ADDRESS        address;
	SECURITY_ATTRIBUTES sa;

	sprintf(DeviceName, "\\\\.\\PHYSICALDRIVE%d", buffer->disknumber);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	hDevice = CreateFile(
		DeviceName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}
	else
	{
		/* TODO: log */
	}

	bRet = DeviceIoControl(hDevice,
		IOCTL_SCSI_GET_ADDRESS,
		&address,
		sizeof(address),
		&address,
		sizeof(address),
		&dwDatalen,
		NULL
	);
	if (!bRet)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}
	else
	{
		/* TODO: log */
	}

	buffer->portnumber = address.PortNumber;
	buffer->pathid = address.PathId;
	buffer->targetid = address.TargetId;
	buffer->lun = address.Lun;

fin:
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDevice);
		hDevice = INVALID_HANDLE_VALUE;
	}

	return dwReturn;
}


DWORD 
GetDiskName(
	VOLUME_INFO* buffer
)
{
	DWORD  dwReturn = DISK_ERROR_SUCCESS;
	DWORD  dwDatalen = 0;
	DWORD  dwStatus = 0;
	int    nCnt1 = 0;
	int    nCnt2 = 0;
	int    nSize = 0;
	BOOL   bRet = TRUE;
	HANDLE hScsi = INVALID_HANDLE_VALUE;
	char   ScsiName[PATH_LEN];
	char* pt = NULL;
	char* name = NULL;
	char* TempName = NULL;
	SECURITY_ATTRIBUTES   sa;
	SCSI_ADAPTER_BUS_INFO* adapterbusinfo = NULL;
	SCSI_INQUIRY_DATA* inq = NULL;
	INQUIRYDATA* inqdata = NULL;

	sprintf(ScsiName, "\\\\.\\Scsi%d:", buffer->portnumber);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	hScsi = CreateFile(
		ScsiName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hScsi == INVALID_HANDLE_VALUE)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}
	else
	{
		/* TODO: log */
	}

	nSize = 1024;
	adapterbusinfo = (SCSI_ADAPTER_BUS_INFO*)GlobalAlloc(GPTR, nSize);
	if (adapterbusinfo == NULL)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}

	while (TRUE)
	{
		bRet = DeviceIoControl(hScsi,
			IOCTL_SCSI_GET_INQUIRY_DATA,
			NULL,
			0,
			adapterbusinfo,
			nSize,
			&dwDatalen,
			NULL
		);
		if (bRet)
		{
			break;
		}

		dwStatus = GetLastError();
		if (dwStatus == ERROR_INSUFFICIENT_BUFFER)
		{
			nSize *= 2;
			GlobalFree(adapterbusinfo);
			adapterbusinfo = NULL;

			adapterbusinfo = (SCSI_ADAPTER_BUS_INFO*)GlobalAlloc(GPTR, nSize);
			if (adapterbusinfo == NULL)
			{
				dwReturn = DISK_ERROR_OTHER;
				goto fin;
			}
		}
		else
		{
			dwReturn = DISK_ERROR_OTHER;
			goto fin;
		}
	}

	for (nCnt1 = 0; nCnt1 < adapterbusinfo->NumberOfBuses; nCnt1++)
	{
		if (adapterbusinfo->BusData[nCnt1].NumberOfLogicalUnits == 0)
		{
			continue;
		}

		pt = ((char*)(adapterbusinfo)) + adapterbusinfo->BusData[nCnt1].InquiryDataOffset;
		inq = (SCSI_INQUIRY_DATA*)pt;

		while (TRUE)
		{
			if (inq->PathId == buffer->pathid && inq->TargetId == buffer->targetid && inq->Lun == buffer->lun)
			{
				inqdata = (INQUIRYDATA*)inq->InquiryData;

				name = (char*)GlobalAlloc(GPTR, sizeof(inqdata->VendorId) + sizeof(inqdata->ProductId) + sizeof(inqdata->ProductRevisionLevel) + 1);
				if (name == NULL)
				{
					dwReturn = DISK_ERROR_OTHER;
					goto fin;
				}

				memset(name, 0, sizeof(inqdata->VendorId) + sizeof(inqdata->ProductId) + sizeof(inqdata->ProductRevisionLevel) + 1);
				memcpy(name, inqdata->VendorId, sizeof(inqdata->VendorId));
				memcpy(&name[strlen(name)], inqdata->ProductId, sizeof(inqdata->ProductId));
				memcpy(&name[strlen(name)], inqdata->ProductRevisionLevel, sizeof(inqdata->ProductRevisionLevel));
				name[strlen(name)] = '\0';
				break;
			}
			if (inq->NextInquiryDataOffset == 0) {
				break;
			}
			pt = ((char*)(adapterbusinfo)) + inq->NextInquiryDataOffset;
			inq = (SCSI_INQUIRY_DATA*)pt;
		}
	}
	if (name == NULL)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}

	strncpy(buffer->diskname, name, DISKNAME_LEN);


fin:
	if (adapterbusinfo != NULL)
	{
		GlobalFree(adapterbusinfo);
		adapterbusinfo = NULL;
	}
	if (name != NULL)
	{
		GlobalFree(name);
		name = NULL;
	}
	if (TempName != NULL)
	{
		GlobalFree(TempName);
		TempName = NULL;
	}
	if (hScsi != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hScsi);
		hScsi = INVALID_HANDLE_VALUE;
	}

	return dwReturn;
}


DWORD 
GetVolumeMountPoint(
	char* VolumeGuid,
	char* MountPoint
)
{
	DWORD dwReturn = DISK_ERROR_OTHER;
	DWORD dwSysFlags = 0;
	DWORD dwDatalen = 0;
	int   nRet = 0;
	int   nSize = 0;
	char* pt = NULL;
	char  VolumeName[PATH_LEN];
	char  Path[PATH_LEN];


	memset(MountPoint, '\0', PATH_LEN);

	sprintf(VolumeName, "\\\\?\\Volume{%s}\\", VolumeGuid);
	nRet = GetVolumePathNamesForVolumeName(VolumeName, Path, PATH_LEN, &dwDatalen);
	if (nRet == 0)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}
	else
	{
		/* TODO: log */
	}

	pt = (char*)& Path[0];
	nSize = 0;
	while (TRUE)
	{
		strcat(MountPoint, pt);
		pt = strchr(pt, '\0');
		if (pt == NULL || *(++pt) == '\0')
		{
			break;
		}
		if (strlen(MountPoint) + strlen(pt) + 3 >= PATH_LEN)
		{
			break;
		}
		strcat(MountPoint, " / ");
	}

fin:
	return dwReturn;
}


BOOL 
IsRemovableVolume(
	char* mountpoint
)
{
	BOOL    bRet = FALSE;
	UINT    uType = 0;

	uType = GetDriveType(mountpoint);
	if (uType == DRIVE_REMOVABLE) {
		bRet = TRUE;
	}

	return bRet;
}


LONG
RegOpenDriverKey(PHKEY hKeyHba, PHKEY hKeyVol)
{
	HKEY hKey, hSubKey;
	LONG lRet;
	CHAR szKey[MAX_PATH];

	/* open a registry key of clpdiskfltr */
	sprintf(szKey, "SYSTEM\\CurrentControlSet\\Services\\%s", "clpdiskfltr");
	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_ALL_ACCESS, &hKey);
	if (lRet != ERROR_SUCCESS) {
		printf("%5d: RegQueryValueEx() failed (%d).\n", __LINE__, lRet);
		return lRet;
	}

	/* create Parameters key */
	lRet = RegCreateKeyEx(hKey, "Parameters", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_ALREADY_EXISTS) {
		printf("%5d: RegCreateKeyEx() failed (%d).\n", __LINE__, lRet);
		RegCloseKey(hKey);
		return lRet;
	}
	RegCloseKey(hSubKey);

	/* create HBAList key */
	lRet = RegCreateKeyEx(hKey, "HBAList", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_ALREADY_EXISTS) {
		printf("%5d: RegCreateKeyEx() failed (%d).\n", __LINE__, lRet);
		RegCloseKey(hKey);
		return lRet;
	}
	RegCloseKey(hSubKey);

	/* create HBAPortList key */
	lRet = RegCreateKeyEx(hKey, "HBAPortList", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_ALREADY_EXISTS) {
		RegCloseKey(hKey);
		return lRet;
	}
	*hKeyHba = hSubKey;

	/* create UniqueIDList key */
	lRet = RegCreateKeyEx(hKey, "UniqueIDList", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_ALREADY_EXISTS) {
		printf("%5d: RegCreateKeyEx() failed (%d).\n", __LINE__, lRet);
		RegCloseKey(*hKeyHba);
		RegCloseKey(hKey);
		return lRet;
	}
	*hKeyVol = hSubKey;
	RegCloseKey(hKey);

	return ERROR_SUCCESS;
}


void
PrintHelp(
	void
)
{
	printf(" Usage: \n");
	printf(" <Get HBA>\n");
	printf("  C:\\> clpctrldisk.exe get hba [drive letter] \n");
	printf("  Example:\n");
	printf("  C:\\> clpctrldisk.exe get hba S:\\ \n\n");
	printf(" <Get GUID>\n");
	printf("  C:\\> clpctrldisk.exe get guid [drive letter] \n");
	printf("  Example:\n");
	printf("  C:\\> clpctrldisk.exe get guid S:\\ \n\n");
	printf(" <Set filetring>\n");
	printf("  C:\\> clpctrldisk.exe set filter [drive letter] \n");
	printf("  Example:\n");
	printf("  C:\\> clpctrldisk.exe set filter S:\\ \n\n");
}
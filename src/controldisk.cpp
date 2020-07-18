#include <Windows.h>
#include <ntddscsi.h>
#include <stdio.h>

#include "controldisk.h"

/*
 * ClpsdfltrInfo.c/GetHbaInfo()
 */
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
		printf("ERROR: Invalid parameter.\n");
		PrintHelp();
		return 1;
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
			printf("ERROR: 2nd argument is invalid.\n");
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
			printf("ERROR: 2nd argument is invalid.\n");
			PrintHelp();
			return DISK_ERROR_PARAMETER;
		}
	}
	else
	{
		printf("ERROR: 1st argument is invalid.\n");
		PrintHelp();
		return DISK_ERROR_PARAMETER;
	}

	/* copy drive letter */
	strcpy(drive, argv[3]);

	/* get system drive */
	if (GetSystemDirectory(szSystemDrive, MAX_PATH + 1) == 0) {
		printf("GetSystemDirectory() failed.\n");
		return DISK_ERROR_QUERY;
	}
	szSystemDrive[2] = '\0';

	/* get computer name */
	bRet = GetComputerNameEx(ComputerNameDnsHostname, hostname, &hostnamelen);
	/* FIXME */

	/* get HBA list */
	dwRet = QueryHbaList(NULL, &size);
	if (dwRet != DISK_ERROR_SUCCESS) {
		printf("QueryHbaList() failed (ret: %d).", dwRet);
		return DISK_ERROR_QUERY;
	}
	lpHba = (LP_HBA_LIST)GlobalAlloc(GPTR, size);
	if (lpHba == NULL) {
		printf("GlobalAlloc() failed (ret: %d).", GetLastError());
		return DISK_ERROR_ALLOCATE;
	}
	dwRet = QueryHbaList(lpHba, &size);
	if (dwRet != DISK_ERROR_SUCCESS) {
		printf("QueryHbaList() failed (ret: %d).", dwRet);
		GlobalFree(lpHba);
		lpHba = NULL;
		return DISK_ERROR_QUERY;
	}

	/* get volume list */
	dwRet = QueryVolumeList(NULL, &size);
	/* FIXME: error handling */
	lpVol = (LP_VOLUME_LIST)GlobalAlloc(GPTR, size);
	if (lpVol == NULL) {
		printf("GlobalAlloc() failed (ret: %d).", GetLastError());
		GlobalFree(lpHba);
		return DISK_ERROR_ALLOCATE;
	}
	dwRet = QueryVolumeList(lpVol, &size);
	if (dwRet != DISK_ERROR_SUCCESS) {
		printf("QueryVolumeList() failed (ret: %d).", dwRet);
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

	/*  */
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
					/* FIXME */
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
					/* FIXME */
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
					/* FIXME */
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
				return 1;
			}
			dwPort = (DWORD)nPort;
			sprintf(cPort, "%d", nPort);
			lRet = RegSetValueEx(hKeyHba, cPort, 0, REG_DWORD, (const BYTE*)&dwPort, sizeof(dwPort));
			if (lRet != ERROR_SUCCESS) 
			{
				/* FIXME */
				return 1;
			}

		}
	}
	
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

	// HBA���̎擾
	nHbaNum = 0;
	for (nCnt = 0; nCnt < MAX_PORT_NUM; nCnt++)
	{
		// �n���h����������
		hScsi[nCnt] = INVALID_HANDLE_VALUE;

		// SCSI���̎擾
		sprintf(ScsiName, "\\\\.\\Scsi%d:", nCnt);

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		// �f�o�C�X���I�[�v��
		hScsi[nCnt] = CreateFile(
			ScsiName,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		//## �I�[�v���ɐ���������f�o�C�X�� ##
		//## ���݂���ƍl����               ##
		if (hScsi[nCnt] != INVALID_HANDLE_VALUE)
		{
			nHbaNum++;
		}
		else
		{
		}
	}

	// �T�C�Y�̎擾
	nSize = sizeof(HBA_LIST_ENTRY) * nHbaNum + sizeof(HBA_LIST);

	// NULL�`�F�b�N
	if (buffer == (HBA_LIST*)NULL)
	{   // �T�C�Y���i�[����̂�
		goto fin;
	}

	if (*size < nSize)
	{   // �T�C�Y���i�[����̂�
		dwReturn = DISK_ERROR_BUFFER;

		goto fin;
	}

	// HBA���X�g�����i�[
	buffer->hbalistnum = nHbaNum;

	// LISCAL_CTRL���I�[�v��
	hLiscalCtrl = CreateFile(
//		LISCAL_CTRL_DOSDEVICE_NAME,
		"\\\\.\\LISCAL_CTRL",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hLiscalCtrl == INVALID_HANDLE_VALUE)
	{   //## ���s���Ă������͌p������ ##
//		printf("LISCAL_CTRL",);
	}
	else
	{
	}

	nHbaNum = 0;
	for (nCnt = 0; nCnt < MAX_PORT_NUM; nCnt++)
	{
		if (hScsi[nCnt] == INVALID_HANDLE_VALUE)
		{
			continue;
		}

		// �ϐ�������
		memset(HbaName, '\0', HBANAME_LEN);
		memset(&HbaInfo, '\0', sizeof(HBA_INFO));

		// HBA�����擾
		//## ���s���Ă������͌p������ ##
		dwRet = GetHbaName(nCnt, HbaName);
		if (dwRet == 0)
		{
			flag = 1;
		}
		else
		{
			flag = 0;
		}

		// �p�����[�^�ݒ�
		HbaInfo.port = nCnt;

		// �f�o�C�X�����擾
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
				// �f�o�C�X���̎擾�Ɏ��s
				//## ���s���Ă������͌p������ ##
			}

			if (HbaInfo.error != 0)
			{
				// �f�o�C�X���̎擾�Ɏ��s
				//## ���s���Ă������͌p������ ##
			}
		}

		// HBA�����i�[
		buffer->hbalist[nHbaNum].portnumber = HbaInfo.port;
		strncpy(buffer->hbalist[nHbaNum].hbaname, HbaName, HBANAME_LEN);
		strncpy(buffer->hbalist[nHbaNum].deviceid, HbaInfo.device_id, HBADEVICEID_LEN);
		strncpy(buffer->hbalist[nHbaNum].instanceid, HbaInfo.instance_id, HBAINSTANSID_LEN);

		nHbaNum++;
	}

fin:
	// �K�v�ȃo�b�t�@�T�C�Y���i�[
	*size = sizeof(HBA_LIST_ENTRY) * nHbaNum + sizeof(HBA_LIST);

	// �n���h�����N���[�Y
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
//	TCHAR  DriverName[HBANAME_LEN];
	char  Zero[HBANAME_LEN];
	char* TempName = NULL;
	HKEY  hRestoreKey = NULL;
	char* lpDesc = NULL;

	sprintf(RegPath, "HARDWARE\\DEVICEMAP\\SCSI\\SCSI PORT %d", PortNum);
	nRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegPath, 0, KEY_ALL_ACCESS, &hRestoreKey);
	if (nRet != ERROR_SUCCESS)
	{
		/* TODO: error handling */
	}
	nSize = HBANAME_LEN;
	nRet = RegQueryValueEx(hRestoreKey, "Driver", NULL, &dwType, DriverName, &nSize);
	if (nRet != ERROR_SUCCESS)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto exit;
	}

	// ���W�X�g�����N���[�Y
	if (hRestoreKey != NULL)
	{
		RegCloseKey(hRestoreKey);
		hRestoreKey = NULL;
	}

	// ���W�X�g���p�X�̍쐬
	sprintf(RegPath, "SYSTEM\\CurrentControlSet\\Services\\%s\\Enum", DriverName);

	// ���W�X�g���̃I�[�v��
	nRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegPath, 0, KEY_ALL_ACCESS, &hRestoreKey);
	if (nRet != ERROR_SUCCESS)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto exit;
	}

	// �f�o�C�X���擾�p�̃p�X���擾
	nSize = HBANAME_LEN;
	nRet = RegQueryValueEx(hRestoreKey, "0", NULL, &dwType, (LPBYTE)Zero, &nSize);
	if (nRet != ERROR_SUCCESS)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto exit;
	}

	// ���W�X�g�����N���[�Y
	if (hRestoreKey != NULL)
	{
		RegCloseKey(hRestoreKey);
		hRestoreKey = NULL;
	}

	// ���W�X�g���p�X�̍쐬
	sprintf(RegPath, "SYSTEM\\CurrentControlSet\\Enum\\%s", Zero);

	// ���W�X�g�����I�[�v��
	//## "KEY_ALL_ACCESS"�ŃI�[�v������� ##
	//## ���s���邽��"KEY_QUERY_VALUE"    ##
	nRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegPath, 0, KEY_QUERY_VALUE, &hRestoreKey);
	if (nRet != ERROR_SUCCESS)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto exit;
	}

	// HBA�����擾
	nSize = HBANAME_LEN;
	nRet = RegQueryValueEx(hRestoreKey, "DeviceDesc", NULL, &dwType, (LPBYTE)HbaName, &nSize);
	if (nRet != ERROR_SUCCESS)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto exit;
	}

	//add 2008/02/25
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
	// ���������J��
	if (TempName != NULL)
	{
		GlobalFree(TempName);
		TempName = NULL;
	}
	// ���W�X�g�����N���[�Y
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

	// �{�����[���̐����擾
	nVolNum = 0;
	while (TRUE)
	{
		// �o�b�t�@�̏�����
		memset(&TempBuff, '\0', sizeof(VOLUME_LIST_ENTRY));

		// �{�����[��GUID�̎擾
		pt1 = strchr(VolumeName, '{');
		pt2 = strchr(VolumeName, '}');
		memset(TempBuff.volumeguid, '\0', PATH_LEN);
		strncpy(TempBuff.volumeguid, pt1 + 1, pt2 - pt1 - 1);

		// �{�����[�����̎擾
		//## disknumber,partitionnumber,partitionsize_hi,partitionsize_lo���擾 ##
		nRet = GetVolumeInfo(&(TempBuff.volumeinfo), TempBuff.volumeguid);
		if (nRet != DISK_ERROR_SUCCESS)
		{
			//## ���s���Ă������͌p������ ##
			nRet = FindNextVolume(hVolume, VolumeName, PATH_LEN);
			if (nRet == 0)
			{
				break;
			}
			continue;
		}

		// �f�B�X�N���̎擾
		//## portnumber,pathid,targetid,lun���擾 ##
		nRet = GetDiskInfo(&(TempBuff.volumeinfo));

		// �f�B�X�N���̎擾
		//## diskname���擾 ##
		//## GetDiskInfo()�ŃG���[������������X���[ ##
		if (nRet == DISK_ERROR_SUCCESS)
		{
			nRet = GetDiskName(&(TempBuff.volumeinfo));
		}

		// �{�����[���}�E���g�|�C���g�̎擾
		//## ���s���Ă��������p�� ##
		nRet = GetVolumeMountPoint(TempBuff.volumeguid, TempBuff.volumemountpoint);

		bSts = IsRemovableVolume(TempBuff.volumemountpoint);
		if (bSts == TRUE) {
			/* RemovalMedia (do nothing) */
		}
		else {
			// �T�C�Y���m�F���A�f�[�^���i�[
			nSize = sizeof(VOLUME_LIST_ENTRY) * (nVolNum + 1) + sizeof(VOLUME_LIST);
			if (buffer != (VOLUME_LIST*)NULL && *size >= nSize)
			{
				buffer->volumelistnum = nVolNum + 1;
				memcpy(&(buffer->volumelist[nVolNum]), &TempBuff, sizeof(VOLUME_LIST_ENTRY));
			}
			// �{�����[�������{�{
			nVolNum++;
		}

		// ���̃{�����[��������
		nRet = FindNextVolume(hVolume, VolumeName, PATH_LEN);
		if (nRet == 0)
		{
			break;
		}
	}

	// �T�C�Y�̎擾
	nSize = sizeof(VOLUME_LIST_ENTRY) * nVolNum + sizeof(VOLUME_LIST);

	// NULL�`�F�b�N
	if (buffer == (VOLUME_LIST*)NULL)
	{   // �T�C�Y���i�[����̂�
//		printf("buffer is NULL. neccesary size : %d\n", nSize);

		goto fin;
	}

	if (*size < nSize)
	{   // �T�C�Y���i�[����̂�
		dwReturn = DISK_ERROR_BUFFER;
//		printf("size is not enough. size : %d, neccesary size : %d\n", *size, nSize);

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

	// �{�����[�����̎擾
	sprintf(VolumeName, "\\\\?\\Volume{%s}", VolumeGuid);

	// �{�����[�����I�[�v��
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

	// �f�B�X�N�����擾
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

	// �{�����[�������擾
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

	// �f�B�X�N��LENGTH�����擾
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
	{   //## ���s���Ă��G���[�Ƃ��Ȃ�            ##
		//## RAW�p�[�e�B�V�����Ŏ��s����ꍇ���� ##

		// LISCAL_CTRL���I�[�v��
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
		{   //## ���s���Ă������͌p������ ##
		}
		else
		{
			// �p�����[�^�ݒ�
			OnlineParam.flag = PG_PORT_IO;
			strncpy(OnlineParam.dp_vmp, VolumeGuid, GUID_LEN);

			// �{�����[�������J��
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
				// ���J���Ɏ��s
			}
			else if (OnlineParam.error != 0)
			{
				// ���J���Ɏ��s
			}
			else
			{
				// �f�B�X�N��LENGTH�����擾
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

	// �擾���������i�[
	buffer->disknumber = extents.Extents[0].DiskNumber;
	buffer->partitionnumber = partitioninfo.PartitionNumber;
	buffer->partitionsize_hi = length.Length.HighPart;
	buffer->partitionsize_lo = length.Length.LowPart;

fin:
	// �n���h�����N���[�Y
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

	

	// �f�o�C�X���̎擾
	sprintf(DeviceName, "\\\\.\\PHYSICALDRIVE%d", buffer->disknumber);

	// �f�o�C�X���I�[�v��
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

	// �f�o�C�X�����擾
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

	// �f�o�C�X�����i�[
	buffer->portnumber = address.PortNumber;
	buffer->pathid = address.PathId;
	buffer->targetid = address.TargetId;
	buffer->lun = address.Lun;

fin:
	// �n���h�����N���[�Y
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


	// SCSI���̎擾
	sprintf(ScsiName, "\\\\.\\Scsi%d:", buffer->portnumber);

	// �f�o�C�X���I�[�v��
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

	// SCSI���̗̈���m��
	nSize = 1024;
	adapterbusinfo = (SCSI_ADAPTER_BUS_INFO*)GlobalAlloc(GPTR, nSize);
	if (adapterbusinfo == NULL)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}

	// SCSI�̌������[�v(�T�C�Y�̓��I�m�ۂ̂���)
	while (TRUE)
	{
		// SCSI���̎擾
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
		{   // �����Ȃ烋�[�v�𔲂���
			break;
		}

		dwStatus = GetLastError();
		if (dwStatus == ERROR_INSUFFICIENT_BUFFER)
		{
			// �̈�̍Ċm��
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

	// BUS���Ń��[�v
	for (nCnt1 = 0; nCnt1 < adapterbusinfo->NumberOfBuses; nCnt1++)
	{
		if (adapterbusinfo->BusData[nCnt1].NumberOfLogicalUnits == 0)
		{   // �������j�b�g����0�Ȃ玟��BUS��
			continue;
		}

		// InquiryData���擾
		pt = ((char*)(adapterbusinfo)) + adapterbusinfo->BusData[nCnt1].InquiryDataOffset;
		inq = (SCSI_INQUIRY_DATA*)pt;

		// ���[�v
		while (TRUE)
		{
			// PathID,TargetID,LUN����v���邩�ۂ�
			if (inq->PathId == buffer->pathid && inq->TargetId == buffer->targetid && inq->Lun == buffer->lun)
			{
				// InquiryData���擾
				inqdata = (INQUIRYDATA*)inq->InquiryData;

				// �f�o�C�X���̗̈���m��
				name = (char*)GlobalAlloc(GPTR, sizeof(inqdata->VendorId) + sizeof(inqdata->ProductId) + sizeof(inqdata->ProductRevisionLevel) + 1);
				if (name == NULL)
				{
					dwReturn = DISK_ERROR_OTHER;
					goto fin;
				}

				// �f�o�C�X���̎擾
				memset(name, 0, sizeof(inqdata->VendorId) + sizeof(inqdata->ProductId) + sizeof(inqdata->ProductRevisionLevel) + 1);
				memcpy(name, inqdata->VendorId, sizeof(inqdata->VendorId));
				memcpy(&name[strlen(name)], inqdata->ProductId, sizeof(inqdata->ProductId));
				memcpy(&name[strlen(name)], inqdata->ProductRevisionLevel, sizeof(inqdata->ProductRevisionLevel));

				name[strlen(name)] = '\0';

				break;
			}

			// ����InquiryData�����݂��邩�ۂ�
			if (inq->NextInquiryDataOffset == 0) {
				break;
			}

			// ����InquiryData���擾
			pt = ((char*)(adapterbusinfo)) + inq->NextInquiryDataOffset;
			inq = (SCSI_INQUIRY_DATA*)pt;
		}
	}
	if (name == NULL)
	{
		dwReturn = DISK_ERROR_OTHER;
		goto fin;
	}

	// DISK�����i�[
	strncpy(buffer->diskname, name, DISKNAME_LEN);


fin:
	// ���������J��
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
	// �n���h�����N���[�Y
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


	// MountPoint�̏�����
	memset(MountPoint, '\0', PATH_LEN);

	// �{�����[�����̎擾
	sprintf(VolumeName, "\\\\?\\Volume{%s}\\", VolumeGuid);

	// �{�����[���p�X���擾
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

	// MountPoint�̎擾
	pt = (char*)& Path[0];
	nSize = 0;
	while (TRUE)
	{
		strcat(MountPoint, pt);

		// '\0'�̌���
		pt = strchr(pt, '\0');

		//## '\0'���Ȃ��Ȃ烋�[�v�𔲂���       ##
		//## �܂��́A����'\0'�Ȃ烋�[�v�𔲂��� ##
		if (pt == NULL || *(++pt) == '\0')
		{
			break;
		}
		// MountPoint�̃T�C�Y�`�F�b�N
		//## MountPoint���I�[�o�t���[����Ȃ� ##
		//## ���[�v�𔲂���                   ##
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

	/* skip removable volume */
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

	/* �f�B�X�N�h���C�o�̃L�[���I�[�v�� */
	sprintf(szKey, "SYSTEM\\CurrentControlSet\\Services\\%s", "clpdiskfltr");
	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_ALL_ACCESS, &hKey);
	if (lRet != ERROR_SUCCESS) {
		return lRet;
	}

	/* Parameters�̃L�[�𐶐� */
	lRet = RegCreateKeyEx(hKey, "Parameters", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_ALREADY_EXISTS) {
		RegCloseKey(hKey);
		return lRet;
	}
	RegCloseKey(hSubKey);

	/* HBAList�̃L�[�𐶐� */
	lRet = RegCreateKeyEx(hKey, "HBAList", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_ALREADY_EXISTS) {
		RegCloseKey(hKey);
		return lRet;
	}
	RegCloseKey(hSubKey);

	/* HBAPortList�̃L�[�𐶐� */
	lRet = RegCreateKeyEx(hKey, "HBAPortList", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_ALREADY_EXISTS) {
		RegCloseKey(hKey);
		return lRet;
	}
	*hKeyHba = hSubKey;

	/* UniqueIDList�̃L�[�𐶐� */
	lRet = RegCreateKeyEx(hKey, "UniqueIDList", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_ALREADY_EXISTS) {
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
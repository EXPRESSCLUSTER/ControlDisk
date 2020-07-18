
/*
 * return value
 */
enum _func_retval__
{
	DISK_ERROR_SUCCESS,
	DISK_ERROR_PARAMETER,
	DISK_ERROR_QUERY,
	DISK_ERROR_ALLOCATE,
	DISK_ERROR_BUFFER,
	DISK_ERROR_OTHER
};


/*
 * macro
 */
#define MAX_PORT_NUM		16
#define PATH_LEN			1024			/* path */
#define GUID_LEN			64				/* GUID */
#define HBANAME_LEN			256				/* HBA name */
#define HBADEVICEID_LEN		256				/* HBA device id */
#define HBAINSTANSID_LEN	256				/* HBA instans id */
#define DISKNAME_LEN		256				/* DISK name */
#define HBADEVICEID_LEN		256				/* HBA device id */
#define HBAINSTANSID_LEN	256				/* HBA instans id */

#define LISCAL_FILE_DEVICE          0x00009F8F
#define MAKE_CTL(f, m) \
        (DWORD)CTL_CODE(LISCAL_FILE_DEVICE, f, m, FILE_ANY_ACCESS)
#define LISCAL_OPEN_DRV_PORT            MAKE_CTL(0x0806, METHOD_BUFFERED)
#define LISCAL_GET_HBA_ID               MAKE_CTL(0x081B, METHOD_BUFFERED)

#define PG_PORT_IO      1   /* IO port */
#define PG_PORT_COM     2   /* COM port */ 
#define PG_PORT_PRE     4   /* Prepare for port */
#define PG_PORT_ALL     PG_PORT_IO | PG_PORT_COM



/*
 * struct
 */

typedef struct _HBA_INFO {
	int     error;
	int     port;
	char    device_id[HBADEVICEID_LEN];
	char    instance_id[HBAINSTANSID_LEN];
} HBA_INFO, * LP_HBA_INFO;


typedef struct _HBA_LIST_ENTRY {
	char hbaname[HBANAME_LEN];
	char deviceid[HBADEVICEID_LEN];
	char instanceid[HBAINSTANSID_LEN];
	unsigned int portnumber;
} HBA_LIST_ENTRY, * LP_HBA_LIST_ENTRY;

typedef struct _HBA_LIST {
	int  hbalistnum;
	HBA_LIST_ENTRY hbalist[];
} HBA_LIST, * LP_HBA_LIST;

typedef struct _VOLUME_INFO {
	char diskname[DISKNAME_LEN];
	unsigned int disknumber;
	unsigned int portnumber;
	unsigned int pathid;
	unsigned int targetid;
	unsigned int lun;
	unsigned int partitionnumber;
	unsigned int partitionsize_lo;
	int  partitionsize_hi;
} VOLUME_INFO, * LP_VOLUME_INFO;

typedef struct _VOLUME_LIST_ENTRY {
	char volumeguid[GUID_LEN];
	char volumemountpoint[PATH_LEN];
	VOLUME_INFO volumeinfo;
} VOLUME_LIST_ENTRY, * LP_VOLUME_LIST_ENTRY;

typedef struct _VOLUME_LIST {
	int  volumelistnum;
	VOLUME_LIST_ENTRY volumelist[];
} VOLUME_LIST, * LP_VOLUME_LIST;


/* PG */
typedef struct
{
	BOOLEAN isIOOpen;   // 活性/非活性状態
	BOOLEAN isCOMOpen;  // ミラーコネクトの状態
}PG_NMP_PORT_INFO, * LPPG_NMP_PORT_INFO;

typedef struct {
	int                 error;                  // エラーステータス
	int                 flag;                   // 口開けしたいPORT
	int                 WaitTime;               // 口閉じまでの猶予時間
	PG_NMP_PORT_INFO    port_info;              // 各PORTの状態
	char                dp_vmp[GUID_LEN];   // 状態を確認したいボリュームのGUID
}PG_DRIVER_PORT_OPERATE, * LPPG_DRIVER_PORT_OPERATE;

typedef struct _INQUIRYDATA {
	UCHAR DeviceType : 5;
	UCHAR DeviceTypeQualifier : 3;
	UCHAR DeviceTypeModifier : 7;
	UCHAR RemovableMedia : 1;
	UCHAR Versions;
	UCHAR ResponseDataFormat : 4;
	UCHAR HiSupport : 1;
	UCHAR NormACA : 1;
	UCHAR ReservedBit : 1;
	UCHAR AERC : 1;
	UCHAR AdditionalLength;
	UCHAR Reserved[2];
	UCHAR SoftReset : 1;
	UCHAR CommandQueue : 1;
	UCHAR Reserved2 : 1;
	UCHAR LinkedCommands : 1;
	UCHAR Synchronous : 1;
	UCHAR Wide16Bit : 1;
	UCHAR Wide32Bit : 1;
	UCHAR RelativeAddressing : 1;
	UCHAR VendorId[8];
	UCHAR ProductId[16];
	UCHAR ProductRevisionLevel[4];
	UCHAR VendorSpecific[20];
	UCHAR Reserved3[40];
} INQUIRYDATA, * PINQUIRYDATA;


/*
 * prototype
 */
DWORD QueryHbaList(HBA_LIST* buffer, int* size);
DWORD QueryVolumeList(VOLUME_LIST* buffer, int* size);
DWORD GetHbaName(int PortNum, char* HbaName);
DWORD GetVolumeInfo(VOLUME_INFO* buffer, char* VolumeGuid);
DWORD GetDiskInfo(VOLUME_INFO* buffer);
DWORD GetDiskName(VOLUME_INFO* buffer);
DWORD GetVolumeMountPoint(char* VolumeGuid, char* MountPoint);
BOOL IsRemovableVolume(char* mountpoint);
LONG RegOpenDriverKey(PHKEY, PHKEY);
void PrintHelp(void);
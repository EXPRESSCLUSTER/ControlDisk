# How to Use

## Sample Configuration
```
+--------------------+      +--------------------+
| - Windows Server   |      | - Windows Server   |
| - EXPRESSCLUSTER X |      | - EXPRESSCLUSTER X |
|        +-----+     |      |     +-----+        |
|        | HBA |     |      |     | HBA |        |
|        +--+--+     |      |     +--+--+        |
+-----------|--------+      +--------|-----------+
            |  +------------------+  |
            |  | Shared Disk      |  |
            +--+ +----+---------+ +--+
               | | R: | S:      | |
               | +----+---------+ |
               +------------------+
R: Volume for network partiotion resolution resource of disk method (called Diks NP Resource)
S: Volume for Disk Resource
```

## Set Filter for HBA
1. Connect a shared disk to a server and create some volumes on the shared disk. 
1. Assign a drive letter for the volume.
1. Install EXPRESSCLUSTER X for Windows with silent mode but not reboot OS.
   - Refer to the following documentation for silent mode.
    - https://www.manuals.nec.co.jp/contents/system/files/nec_manuals/node/504/W42_IG_EN/W_IG.html#installing-the-expresscluster-server-in-silent-mode 
1. Run the following command.
   ```bat
   C:\> clpdiskctrl.exe set filter <drive letter (e.g. S:\)>
   ```
   - Run the following command to check if the port number is registered.
     ```bat
     C:\> reg query HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\clpdiskfltr\HBAPortList
     
     HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\clpdiskfltr\HBAPortList
         3    REG_DWORD    0x3
     ```
1. Reboot OS.

## Get HBA information and GUID
### Get HBA Information
1. Run the following command.
   ```bat
   C:\> clpdiskctrl.exe get hba <drive letter (e.g. S:\)>
   ```
1. You get the following file.
   ```bat
   C:\> type <computer name>_hba.csv
   portnumber,deviceid,instanceid
   3,ROOT\ISCSIPRT,0000
   ```
1. Set **portnumber**, **deviceid** and **instanceid** as below.
   ```xml
   <server name="your hostname">
     :
           <hba id="0">
                   <portnumber>0</portnumber>
                   <deviceid>ROOT\ISCSIPRT</deviceid>
                   <instanceid>0000</instanceid>
           </hba>
   ```

### Get GUID
1. Run the following command.
   ```bat
   C:\> clpdiskctrl.exe get guid <drive letter (e.g. S:\)>
   ```
1. You get the following file.
   ```bat
   C:\> type <computer name>_guid.csv
   volumeguid,drive
   7ea7a3d1-4e40-4050-a2e4-6751c147dee0,S:\
   ```
1. Set **volumeguid** and **drive** as below.
   - Disk Resource
     - Set **volumeguid** and **volumemountpoint** as below.
       ```xml
       <resource>
               <types name="sd"/>
               <sd name="sd">
                       <comment> </comment>
                       <parameters>
                               <volumemountpoint>S:</volumemountpoint>
                       </parameters>
                       <server name="your hostname">
                               <parameters>
                                       <volumeguid>7ea7a3d1-4e40-4050-a2e4-6751c147dee0</volumeguid>
                               </parameters>
                       </server>
               </sd>
       </resource>
       ```
   - Disk NP Resource
     - Set **info** and **extend** as below.
       ```xml
            <server name="your hostname">
             :
                    <device id="10100">
                            <type>disknp</type>
                            <info>e8219479-a1cc-438f-b7d7-f9235b14a9b4</info>
                            <extend>R:\</extend>
                    </device>
            </server>
       ```
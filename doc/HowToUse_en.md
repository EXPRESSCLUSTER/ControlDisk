# How to Use
## Set Filtering for HBA
1. Connect a shared disk to a server and create some volumes on the shared disk. 
1. Assign a drive letter for the volume.
1. Install EXPRESSCLUSTER but not reboot OS.
1. Run the following command.
   ```bat
   C:\> clpctrldisk.exe set filter <drive letter (e.g. S:\)>
   ```
1. Reboot OS.

## Get HBA information and GUID for Cluster Configuration File
1. Run the Get HBA information and GUID

### Get HBA 
1. Run the following command.
   ```bat
   C:\> clpctrldisk.exe get hba <drive letter (e.g. S:\)>
   ```
1. You get the following file.
   ```bat
   C:\> type <computer name>_hba.csv

   ```

### Get GUID
1. Run the following command.
   ```bat
   C:\> clpctrldisk.exe get guid <drive letter (e.g. S:\)>
   ```
1. You get the following file.
   ```bat
   C:\> type <computer name>_guid.csv
   volumeguid,drive
   7ea7a3d1-4e40-4050-a2e4-6751c147dee0,S:\
   ```
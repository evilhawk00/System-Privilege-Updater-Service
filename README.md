# System Privilege Updater Service

A simple example for Creating a Windows Service that performs a software update with SYSTEM privilege at startup.

With this example, software publisher can ensure their software are always up-to-date on some PCs even if the Logged in user is not a system administrator. This concept is also used on some famous software, such as, [Steam](http://store.steampowered.com/about/ "Steam") by Valve Corporation, even user logged in without administrator privilege, they can always keep their installed games up-to-date.

***This is just an example for research and education purpose to proof the mentioned concept, it does not contain the Graphical-User-Interface part which the user have to intergrate by themself. Remember that you should always keep the user notified when updating the software. Downloading files to a PC without agreement is illegal,you should never do this!!!***



###Features

- Update Information are stored as plain text file. Any direct link to a plain text file can fit the needs of a cloud server.
- Version Checker, Compares the version between local and remote.
- Self-Update, Allowed to update itself if necessary
- Uses http protocal with winHTTP, does not support https since this is just an example for research and education purpose.

###What does the client side do?
- The client side checks the update at startup, if update found, download the update installer and execute the installer.


Host side configuration(cloud)
-------------
Create an plain text file with the following example, modify the file and host it, make sure the file is accessible with direct link. We call this file **Update.ini** in the following example(you can have your own name, such as, metadata.txt, ud.ini, ...etc)


    Version=1
    Path=/Update/updateSetupv20180101.exe
    ExtHost=0
    ExtPath=0
    CmdLine=/fix
    DLStoreMode=0
    DLStoreDir=0
    DLStoreFileName=0
    ResetVersion=0
    ResetUniqueID=0
    SelfUpgrade=0

***Configuration parameters:***
 **Version:** The Version of the update, have to be unsigned integer (range: 	1~4294967295),also 0 is not allowed. The client compares this parameter with cloud to decide if the cloud have a newer patch. The bigger number means the newer version.(Example: Local have Version=1, remote have Version=3, client downloads the update; Local have Version=5, remote have Version=2, client will not do anything.)

**Path(optional):** The FilePath to the Target Update patch file. If the Target Update patch file is hosted on the same server where **Update.ini** is hosted, you have to configure this parameter correctly and set both parameters **ExtHost** and **ExtPath** to 0. The client will grab the update file with this Path on the same server where **Update.ini** is hosted. If your update patch is hosted on the different server with **Update.ini**, set this parameter to 0.

**ExtHost(optional):** You have **Path** parameter set to 0, this parameter indicates the External host that hosts your update patch installer,do not include protocal HTTP or HTTPS.(For example: download.microsoft.com)

**ExtPath(optional):** You have **Path** parameter set to 0, this parameter indicates the FilePath to your update path installer on the external host.

 **CmdLine(optional):** The command-line parameter for the update patch installer, such as , `/upgrade`, `/c`, `/h`,...etc.  

**DLStoreMode(optional):** If set to 0, the client downloads the update installer and execute it. If set to 1, the client only downloads the update installer and store it to a specific directory without executing it. If this parameter is set to 1, you have to configure both **DLStoreDir** and **DLStoreFileName** parameters to make it work.

**DLStoreDir(optional):** You have **DLStoreMode** parameter set to 1, this parameter indicates the path on local pc where you would like the client to store the downloaded file to. Use this in Windows API format, for example `DLStoreDir=C:\\UpdatePatch\\v1.2\\`.

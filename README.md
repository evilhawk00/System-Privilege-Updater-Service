# System Privilege Updater Service

A simple example for Creating a Windows Service that performs a software update with SYSTEM privilege at startup.

With this example, software publisher can ensure their software are always up-to-date on some PCs even if the Logged in user is not a system administrator. This concept is also used on some famous software, such as, [Steam](http://store.steampowered.com/about/ "Steam") by Valve Corporation, even user logged in without administrator privilege, they can always keep their installed games up-to-date.


------------
**This is just an example for research and education purpose to proof the mentioned concept, it does not contain the Graphical-User-Interface part which the user have to intergrate by themself. Remember that you should always keep the user notified when updating the software. Downloading files to a PC without agreement is illegal,you should never do this!!!**

------------



Features
------------
- Update Information are stored as plain text file. Any direct link to a plain text file can fit the needs of a cloud server.
- Version Checker, Compares the version between local and remote.
- Self-Update, Allowed to update itself if necessary
- Uses http protocal with winHTTP, does not support https since this is just an example for research and education purpose.

What does the client side do?
------------
- The client side checks the update at startup, if newer update found, download the update installer and execute the installer. Then the client records the installed update version at local to ensure it will not install the same update again and again.


Host side configuration(cloud)
-------------
Create an plain text file with the following example, modify the file and host it, make sure the file is accessible with direct link. We call this file **Update.ini** in the following example(you can have your own name, such as, metadata.txt, ud.ini, ...etc)

##### Update.ini

    Version=1
    Path=/Update/updateSetupv20180101.exe
    ExtHost=0
    ExtPath=0
    CmdLine=/my-command-line
    DLStoreMode=0
    DLStoreDir=0
    DLStoreFileName=0
    ResetVersion=0
    ResetUniqueID=0
    SelfUpgrade=0

#### Configuration parameters:

*※※Set a optional parameter to 0 if not used※※*

 **Version:** The Version of the update, have to be unsigned integer (range: 	1~4294967295),also 0 is not allowed. The client compares this parameter with cloud to decide if the cloud have a newer patch. The bigger number means the newer version.(Example: Local have Version=1, remote have Version=3, client downloads the update; Local have Version=5, remote have Version=2, client will not do anything.)

**Path(optional):** The FilePath to the Target Update patch file. If the Target Update patch file is hosted on the same server where **Update.ini** is hosted, you have to configure this parameter correctly and set both parameters **ExtHost** and **ExtPath** to 0. The client will grab the update file with this Path on the same server where **Update.ini** is hosted. If your update patch is hosted on the different server with **Update.ini**, set **Path** to 0.

**ExtHost(optional):** If you have **Path** parameter set to 0, use this parameter as the External host that hosts your update patch installer,do not include protocal HTTP or HTTPS(For example: download.microsoft.com). If not, set **ExtHost** to 0.

**ExtPath(optional):** If you have **Path** parameter set to 0, use this parameter as the FilePath to your update path installer on the external host. If not, set **ExtPath** to 0.

 **CmdLine(optional):** The command-line parameter for the update patch installer, such as , `/upgrade`, `/c`, `/h`,...etc.  If you have no command-line options, set **CmdLine** to 0.

**DLStoreMode(optional):** If set to 0, the client downloads the update installer and execute it. If set to 1, the client only downloads the update installer and store it to a specific directory without executing it. If this parameter is set to 1, you also have to configure both **DLStoreDir** and **DLStoreFileName** parameters so the client knows where to store the downloaded file.

**DLStoreDir(optional):** If you have **DLStoreMode** parameter set to 1, use this parameter as the path on local pc where you would like the client to store the downloaded file to. Use this in Windows API format, use `\\` instead of `\` ,for example `DLStoreDir=C:\\UpdatePatch\\v1.2\\`. If  you have **DLStoreMode** set to 0, set **DLStoreDir** to 0.

**DLStoreFileName(optional):** If you have **DLStoreMode** parameter set to 1, use this parameter as the Filename that the client would use to save the downloaded file. If  you have **DLStoreMode** set to 0, set **DLStoreFileName** to 0.

 **ResetVersion(optional):** Everytime the client installs an update, it records the installed patch **Version** to local, the **Version** record at local gets bigger and bigger after a lot of updates. If set this parameter to 1, the client will reset the local **Version** record to 0 which will allow the cloud to publish updates with smaller **Version** number. To complete a ResetVersion action, you also have to set   **ResetUniqueID** correctly, otherwise the ResetVersion action will not be done. If not used, set **ResetVersion** to 0.

 **ResetUniqueID(optional):** If you have **ResetVersion** parameter set to 1, use this parameter as an unique string for the client to identify if it have already reset the Version record at local. This unique string can contain alphabetical letters and strings, for example: `ResetUniqueID=201801stReset`. If not used, set **ResetUniqueID** to 0.

**SelfUpgrade(optional):** If **SelfUpgrade** is set to 0 (means disabled),the client behaves the normal update procedure, the client downloads the installer, execute the installer and wait until the update installer ends itself, then the client write the new Version record to local and end the client itself. If **SelfUpgrade** is set to 1 (means enabled), the client downloads the inistaller, execute the installer and write the new Version record to local then end the client itself ***without waiting the installer ends*.** This allows the update installer to replace the binary file of the client, making an upgrade of the updater service.


# License
###### The MIT License

Copyright © 2018 CHEN-YAN-LIN

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// 项目自身文件
#include "HWGenerator.h"

// Qt库文件
#include<QDebug>

// 其它头文件
#include "DeviceManager/DeviceManager.h"
#include "DeviceManager/DeviceGpu.h"
#include "DeviceManager/DeviceMonitor.h"
#include "DeviceManager/DeviceOthers.h"
#include "DeviceManager/DeviceStorage.h"
#include "DeviceManager/DeviceAudio.h"
#include "DeviceManager/DeviceComputer.h"
#include "DeviceManager/DevicePower.h"
#include "DeviceManager/DeviceInput.h"
#include "DeviceManager/DeviceBluetooth.h"

HWGenerator::HWGenerator()
{

}

void HWGenerator::generatorAudioDevice()
{
    getAudioInfoFromCatAudio();
}

void HWGenerator::generatorBluetoothDevice()
{
    getBluetoothInfoFromHciconfig();
    getBlueToothInfoFromHwinfo();
    getBluetoothInfoFromLshw();
    getBluetoothInfoFromCatWifiInfo();
}

void HWGenerator::getAudioInfoFromCatAudio()
{
    const QList<QMap<QString, QString>> lstAudio = DeviceManager::instance()->cmdInfo("cat_audio");
    QList<QMap<QString, QString> >::const_iterator it = lstAudio.begin();
    for (; it != lstAudio.end(); ++it) {
        if ((*it).size() < 2)
            continue;

        QMap<QString, QString> tempMap = *it;
        tempMap["driver"] = tempMap["Name"];
        if(tempMap["Name"].contains("da_combine_v5",Qt::CaseInsensitive)){
            tempMap["Name"] = "Hi6405";
            tempMap["Model"] = "Hi6405";
        }

        DeviceAudio *device = new DeviceAudio();
        device->setCanEnale(false);
        device->setCanUninstall(false);
        device->setInfoFromCatAudio(tempMap);
        DeviceManager::instance()->addAudioDevice(device);
    }
}

void HWGenerator::getDiskInfoFromLshw()
{
    QString bootdevicePath("/proc/bootdevice/product_name");
    QString modelStr = "";
    QFile file(bootdevicePath);
    if(file.open(QIODevice::ReadOnly)){
        modelStr = file.readLine().simplified();
        file.close();
    }

    const QList<QMap<QString, QString>> lstDisk = DeviceManager::instance()->cmdInfo("lshw_disk");
    QList<QMap<QString, QString> >::const_iterator dIt = lstDisk.begin();
    for (; dIt != lstDisk.end(); ++dIt) {
        if ((*dIt).size() < 2)
            continue;

        // KLU的问题特殊处理
        QMap<QString, QString> tempMap;
        foreach (const QString &key, (*dIt).keys()) {
            tempMap.insert(key, (*dIt)[key]);
        }

//        qInfo() << tempMap["product"] << " ***** " << modelStr << " " << (tempMap["product"] == modelStr);
        // HW写死
        if (tempMap["product"] == modelStr) {
            // 应HW的要求，将描述固定为   Universal Flash Storage
            tempMap["description"] = "Universal Flash Storage";
            // 应HW的要求，添加interface   UFS 3.0
            tempMap["interface"] = "UFS 3.0";
        }

        DeviceManager::instance()->addLshwinfoIntoStorageDevice(tempMap);
    }
}

void HWGenerator::getDiskInfoFromSmartCtl()
{
    const QList<QMap<QString, QString>> lstMap = DeviceManager::instance()->cmdInfo("smart");
    QList<QMap<QString, QString> >::const_iterator it = lstMap.begin();
    for (; it != lstMap.end(); ++it) {
        if ((*it).size() < 5)
            continue;

        // KLU的问题特殊处理
        QMap<QString, QString> tempMap;
        foreach (const QString &key, (*it).keys()) {
            tempMap.insert(key, (*it)[key]);
        }

        // 按照HW的需求，如果是固态硬盘就不显示转速
        if (tempMap["Rotation Rate"] == "Solid State Device")
            tempMap["Rotation Rate"] = "HW_SSD";

        DeviceManager::instance()->setStorageInfoFromSmartctl(tempMap["ln"], tempMap);
    }
}

void HWGenerator::getBluetoothInfoFromHciconfig()
{
    const QList<QMap<QString, QString>> lstMap = DeviceManager::instance()->cmdInfo("hciconfig");
    QList<QMap<QString, QString> >::const_iterator it = lstMap.begin();
    for (; it != lstMap.end(); ++it) {
        if ((*it).size() < 1) {
            continue;
        }
        DeviceBluetooth *device = new DeviceBluetooth();
        device->setCanEnale(false);
        device->setInfoFromHciconfig(*it);
        DeviceManager::instance()->addBluetoothDevice(device);
    }
}

void HWGenerator::getBlueToothInfoFromHwinfo()
{
    const QList<QMap<QString, QString>> lstMap = DeviceManager::instance()->cmdInfo("hwinfo_usb");
    QList<QMap<QString, QString> >::const_iterator it = lstMap.begin();
    for (; it != lstMap.end(); ++it) {
        if ((*it).size() < 1) {
            continue;
        }
        if ((*it)["Hardware Class"] == "hub" || (*it)["Hardware Class"] == "mouse" || (*it)["Hardware Class"] == "keyboard") {
            continue;
        }
        if ((*it)["Hardware Class"] == "bluetooth" || (*it)["Driver"] == "btusb" || (*it)["Device"] == "BCM20702A0") {
            if (DeviceManager::instance()->setBluetoothInfoFromHwinfo(*it))
                addBusIDFromHwinfo((*it)["SysFS BusID"]);
        }
    }
}

void HWGenerator::getBluetoothInfoFromLshw()
{
    const QList<QMap<QString, QString>> lstMap = DeviceManager::instance()->cmdInfo("lshw_usb");
    QList<QMap<QString, QString> >::const_iterator it = lstMap.begin();
    for (; it != lstMap.end(); ++it) {
        if ((*it).size() < 1) {
            continue;
        }
        DeviceManager::instance()->setBluetoothInfoFromLshw(*it);
    }
}

void HWGenerator::getBluetoothInfoFromCatWifiInfo()
{
    QList<QMap<QString, QString> >  lstWifiInfo;
    QString wifiDevicesInfoPath("/sys/hisys/wal/wifi_devices_info");
    QFile file(wifiDevicesInfoPath);
    if(file.open(QIODevice::ReadOnly)){
        QMap<QString, QString>  wifiInfo;
        QString allStr = file.readAll();
        file.close();

        // 解析数据
        QStringList items = allStr.split("\n");
        foreach (const QString &item, items) {
            if (item.isEmpty())
                continue;

            QStringList strList = item.split(':', QString::SkipEmptyParts);
            if(strList.size() == 2)
                wifiInfo[strList[0] ] = strList[1];
        }

        if(!wifiInfo.isEmpty())
            lstWifiInfo.append(wifiInfo);
    }

    if (lstWifiInfo.size() == 0) {
        return;
    }
    QList<QMap<QString, QString> >::const_iterator it = lstWifiInfo.begin();

    for (; it != lstWifiInfo.end(); ++it) {
        if ((*it).size() < 3) {
            continue;
        }

        // KLU的问题特殊处理
        QMap<QString, QString> tempMap;
        foreach (const QString &key, (*it).keys()) {
            tempMap.insert(key, (*it)[key]);
        }

        // cat /sys/hisys/wal/wifi_devices_info  获取结果为 HUAWEI HI103
        if (tempMap["Chip Type"].contains("HUAWEI", Qt::CaseInsensitive)) {
            tempMap["Chip Type"] = tempMap["Chip Type"].remove("HUAWEI").trimmed();
        }

        // 按照华为的需求，设置蓝牙制造商和类型
        tempMap["Vendor"] = "HISILICON";
        tempMap["Type"] = "Bluetooth Device";

        DeviceManager::instance()->setBluetoothInfoFromWifiInfo(tempMap);
    }
}

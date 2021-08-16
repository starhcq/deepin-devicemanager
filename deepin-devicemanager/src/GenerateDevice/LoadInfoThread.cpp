#include "LoadInfoThread.h"

#include <QDebug>
#include <QDateTime>

#include "GetInfoPool.h"
#include "GenerateDevicePool.h"
#include "ZmqOrder.h"

#include<malloc.h>

LoadInfoThread::LoadInfoThread()
    : mp_ReadFilePool()
    , mp_GenerateDevicePool()
    , mp_ZmqOrder(nullptr)
    , m_Running(false)
    , m_FinishedReadFilePool(false)
    , m_Start(true)
{
    connect(&mp_ReadFilePool, &GetInfoPool::finishedAll, this, &LoadInfoThread::slotFinishedReadFilePool);
}

LoadInfoThread::~LoadInfoThread()
{
    mp_ReadFilePool.deleteLater();
    mp_GenerateDevicePool.deleteLater();
    if (mp_ZmqOrder) {
        delete mp_ZmqOrder;
        mp_ZmqOrder = nullptr;
    }
}


void LoadInfoThread::run()
{
    if (!mp_ZmqOrder) {
        mp_ZmqOrder = new ZmqOrder;
        if (!mp_ZmqOrder->connect()) {
            emit finished("finish");
            m_Running = false;
            return;
        }
    }

    // 请求后台更新信息
    m_Running = true;
    if (mp_ZmqOrder->reqUpdateUI(m_Start)) {
        m_Start = false;
        mp_ReadFilePool.getAllInfo();
        mp_ReadFilePool.waitForDone(-1);

        // 为了保证上面那个线程池完全结束
        long long begin = QDateTime::currentMSecsSinceEpoch();
        while (true) {
            if (m_FinishedReadFilePool)
                break;
            long long end = QDateTime::currentMSecsSinceEpoch();
            if (end - begin > 4000)
                break;
            usleep(100);
        }
        m_FinishedReadFilePool = false;

        mp_GenerateDevicePool.generateDevice();
        mp_GenerateDevicePool.waitForDone(-1);
    }

    emit finished("finish");
    m_Running = false;
}

void LoadInfoThread::slotFinishedReadFilePool(const QString &)
{
    m_FinishedReadFilePool = true;
}

void LoadInfoThread::setFramework(const QString &arch)
{
    // 设置架构
    mp_ReadFilePool.setFramework(arch);
}

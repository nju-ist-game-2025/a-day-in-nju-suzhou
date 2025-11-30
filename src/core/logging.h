#pragma once

#include <QLoggingCategory>
#include <atomic>

namespace Logging {

// 初始化并安装消息处理程序。默认 debugEnabled=false（即关闭 qDebug）。
    void initializeLogging(bool debugEnabled = false);

// 打开或关闭 debug 输出（控制 qDebug、qInfo 等类型为 debug 的输出）
    void setDebugEnabled(bool enabled);

// 查询当前 debug 是否开启
    bool isDebugEnabled();

}  // namespace Logging

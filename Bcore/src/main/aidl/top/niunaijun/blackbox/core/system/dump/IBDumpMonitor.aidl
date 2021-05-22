package top.niunaijun.blackbox.core.system.dump;

import top.niunaijun.blackbox.entity.dump.DumpResult;

interface IBDumpMonitor {
    void onDump(in DumpResult result);
}
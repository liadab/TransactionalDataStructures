#pragma once

struct LocalTransaction {
    uint64_t readVersion = 0L;
    uint64_t writeVersion = 0L; // for debug
    bool TX = false;
    bool readOnly = true;
};


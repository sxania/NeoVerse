// Config.h - runtime settings, loaded from / saved to data/config.txt
#pragma once

struct SystemConfig {
    int maxLoginAttempts     = 3;    // failed logins before the system locks
    int emergencyThreshold   = 4;    // event severity (1-5) at/above which an event is an EMERGENCY
    int criticalAlertLevel   = 4;    // sensor alert level (0-5) at/above which a reading is CRITICAL
    int sensorRetentionDays  = 30;   // sensor readings older than this are "outdated"
    int logRetentionCount    = 500;  // number of newest city logs kept when trimming
    int simulationSeed       = 2035; // random seed so simulations are repeatable
};

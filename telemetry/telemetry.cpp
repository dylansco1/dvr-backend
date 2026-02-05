#include <iostream>

int main() {
    system ("rm -rf /opt/telemetrylatest.txt");
    system("echo 'Telemetry for ID' $(hostname) >> /opt/telemetrylatest.txt");
    system("echo Uptime: >> /opt/telemetrylatest.txt");
    system("echo $(uptime) >> /opt/telemetrylatest.txt");
    system("echo Disk space used >> /opt/telemetrylatest.txt");
    system("echo $(df -h) >> /opt/telemetrylatest.txt");
    system("echo Memory usage >> /opt/telemetrylatest.txt");
    system("echo $(free -h) >> /opt/telemetrylatest.txt"); 
    system("echo CPU load >> /opt/telemetrylatest.txt");
    system("echo $(top -bn1 | grep load) >> /opt/telemetrylatest.txt");
    system("echo Network usage >> /opt/telemetrylatest.txt");
    system("echo $(ifconfig) >> /opt/telemetrylatest.txt");
    system(
    "curl -X POST "
    "-F \"file=@/opt/telemetrylatest.txt\" "
    "https://discordapp.com/api/webhooks/1468980360766554126/velkq5FB2tOr7esNSiJOvLDTddThzwyK0n1P4H6p9-RJL56tJ0hhY-ftaYGER8XJmMG1"
    );

    return 0;
}   
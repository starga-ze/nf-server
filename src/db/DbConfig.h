#pragma once

#include <string>
#include <cstddef>

struct DbConfig {
    std::string server;    // "localhost,1433"
    std::string database;  // "NFENGINE"
    std::string user;      // "admin"
    std::string password;  // "Admin123"
    bool encrypt = true;
    bool trustServerCert = true;
    size_t poolSize = 4;

    std::string buildConnStr() const {
        std::string conn;

        conn += "DRIVER={ODBC Driver 18 for SQL Server};";
        conn += "SERVER=" + server + ";";
        conn += "DATABASE=" + database + ";";
        conn += "UID=" + user + ";";
        conn += "PWD=" + password + ";";

        conn += std::string("Encrypt=") + (encrypt ? "yes;" : "no;");
        conn += std::string("TrustServerCertificate=") +
                (trustServerCert ? "yes;" : "no;");

        return conn;
    }
};


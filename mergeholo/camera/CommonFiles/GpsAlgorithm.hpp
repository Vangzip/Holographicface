#ifndef COMMONALGORITHM_HPP
#define COMMONALGORITHM_HPP

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <chrono>
#include <QString>
#include <QDateTime>

namespace JP::GPSAlgo
{
    class SaveGpsOpt
    {
    public:
        inline bool saveGpsFile(uint8_t *dataBuf, int dataLen)
        {
            if( isDateChange() )
            {
                scrollFile();
            }

            if(!fgps)
            {
                printf("open file to write gps data failed!\n");
                return false;
            }
            else
            {
                printf("open file to write gps data succeed!\n");
            }

            uint8_t buffer[1024] = {0};
            memcpy(buffer, dataBuf, dataLen);

            fgps.write((const char*)buffer, dataLen);

            m_latestDate = QDateTime::currentDateTime();
            return true;
        }
        void closeFile()
        {
            fgps.close();
        }

    private:
        bool isDateChange()
        {
            QDateTime now = QDateTime::currentDateTime();
            if(m_latestDate.date().day() != now.date().day())
            {
                return true;
            }
            return false;
        }

        void scrollFile()
        {
            fgps.close();
            std::time_t tt = std::time(0);
            std::tm *tm = std::localtime(&tt);
            char tmbuf[20];
            std::strftime(tmbuf, sizeof(tmbuf), "%Y_%m_%d", tm);
            QString gpsDate = QString("gps_") + QString::fromStdString(tmbuf) + QString(".txt");
            //    qDebug() << "gps File base name : " << gpsDate ;

            fgps = std::ofstream(gpsDate.toStdString(), std::ios::app | std::ios::out | std::ios::binary);
        }

    private:
        QDateTime m_latestDate;
        std::ofstream fgps;
    };
}

#endif // COMMONALGORITHOM_HPP

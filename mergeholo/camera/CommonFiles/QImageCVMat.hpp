#ifndef QIMAGECVMAT_HPP
#define QIMAGECVMAT_HPP
#include <QImage>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <QFile>
namespace CVHelper {

static void putText( cv::Mat& image, std::string strText )
{

    //设置绘制文本的相关参数

    int font_face = cv::FONT_HERSHEY_COMPLEX;
    double font_scale = 5.0;//*image.rows/256;
    int thickness = 3;
    int baseline;
    //获取文本框的长宽
    cv::Size text_size = cv::getTextSize(strText, font_face, font_scale, thickness, &baseline);

    //将文本框居中绘制
    cv::Point origin;
    origin.x = 200 *font_scale;//image.cols / 2 - text_size.width / 2;
    origin.y = 200 *font_scale + text_size.height;//image.rows / 2 + text_size.height / 2;
    cv::putText(image, strText, origin, font_face, font_scale, cv::Scalar(0, 0, 255), thickness, 4, 0);
}

template< typename T>
static bool matToCsv( cv::Mat& d, std::string strPath )
{
    if( d.empty() )
    {
        return false;
    }

    std::ofstream fout;
    fout.open(strPath);
    if( !fout.is_open() )
    {
        return false;
    }

    for(int i=0; i<d.rows; i++ )
    {
        for( int j=0; j<d.cols; j++ )
        {
            T value = *(d.ptr<T>(i,j));
            if( value >= std::numeric_limits<T>::max()
                    || value <= std::numeric_limits<T>::lowest()
                    ||value == std::numeric_limits<T>::infinity())
            {
                value = 0;
            }
            fout<< value << ",";
        }
        fout << std::endl;
    }

    fout.close();
    return true;
}

template<typename T>
static bool readCsv(cv::Mat& m, QString strCsv )
{
    QFile f(strCsv);
    if( ! f.open(QIODevice::ReadOnly) )
    {
        return false;
    }
    int row = 0;

    while( row < m.rows )
    {
        QString strLine = f.readLine();
        if( strLine.isEmpty() )
        {
            return false;
        }
        QStringList l = strLine.split(",");
        for(int i=0; i<m.cols && i<l.size(); i++ )
        {
            double d = l[i].toDouble();
            m.at<T>(row,i) = static_cast<T>(d);
        }
        row ++;
    }

    return true;
}

static QImage cvMat2QImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        //qDebug() << "CV_8UC4";
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        //qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}
static cv::Mat QImage2cvMat(QImage image)
{
    cv::Mat mat;
    //qDebug() << image.format();
    switch(image.format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
        break;
    case QImage::Format_Indexed8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
        break;
    default:
        break;
    }
    return mat;
}


};

#endif // QIMAGECVMAT_HPP

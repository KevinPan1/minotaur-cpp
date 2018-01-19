#include "camera.h"
#include <QDebug>
#include <QtGui/QPainter>
#include <QtWidgets/QVBoxLayout>

Capture::Capture(QObject *parent)
    : QObject(parent) {}

void Capture::start(int cam) {
    if (!m_video_capture) {
        m_video_capture.reset(new cv::VideoCapture(cam));
    }
    if (m_video_capture->isOpened()) {
        m_timer.start(0, this);
        Q_EMIT started();
    }
}

void Capture::stop() {
    m_timer.stop();
}

void Capture::timerEvent(QTimerEvent *ev) {
    if (ev->timerId() != m_timer.timerId()) {
        return;
    }
    cv::Mat frame;
    if (!m_video_capture->read(frame)) {
        m_timer.stop();
        return;
    }
    Q_EMIT matReady(frame);
}

Converter::Converter(QObject *parent)
    : QObject(parent) {}

void Converter::setProcessAll(bool process_all) {
    m_process_all = process_all;
}

void Converter::processFrame(const cv::Mat &frame) {
    if (m_process_all) {
        process(frame);
    } else {
        queue(frame);
    }
}

void Converter::matDelete(void *mat) {
    delete static_cast<cv::Mat *>(mat);
}

void Converter::queue(const cv::Mat &frame) {
    /*if (m_frame.empty()) {
        qDebug() << "OpenCV Image Converter dropped a frame";
    }*/
    m_frame = frame;
    if (!m_timer.isActive()) {
        m_timer.start(0, this);
    }
}

void Converter::process(cv::Mat frame) {
    cv::resize(frame, frame, cv::Size(), 1, 1, cv::INTER_AREA);
    cv::cvtColor(frame, frame, CV_BGR2RGB);
    const QImage image(
        frame.data, frame.cols, frame.rows, static_cast<int>(frame.step),
        QImage::Format_RGB888, &matDelete, new cv::Mat(frame)
    );
    Q_ASSERT(image.constBits() == frame.data);
    emit imageReady(image);
}

void Converter::timerEvent(QTimerEvent *ev) {
    if (ev->timerId() != m_timer.timerId()) {
        return;
    }
    process(m_frame);
    m_frame.release();
    m_timer.stop();
}

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void ImageViewer::setImage(const QImage &img) {
    /*if (m_img.isNull()) {
        qDebug() << "OpenCV Image Viewer dropped a frame";
    }*/
    m_img = img;
    if (m_img.size() != size()) {
        setFixedSize(m_img.size());
    }
    update();
}

void ImageViewer::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.drawImage(0, 0, m_img);
    m_img = {};
}

IThread::~IThread() {
    quit();
    wait();
}

CameraDisplay::CameraDisplay(QWidget *parent, int camera)
    : QDialog(parent),
      m_camera(camera) {
    m_layout = new QVBoxLayout(this);
    m_image_viewer = new ImageViewer(this);
    setLayout(m_layout);
    m_layout->addWidget(m_image_viewer);
}

CameraDisplay::~CameraDisplay() {
    delete m_layout;
    delete m_image_viewer;
}

void CameraDisplay::setCamera(int camera) {
    m_camera = camera;
}

int CameraDisplay::getCamera() {
    return m_camera;
}

void CameraDisplay::setVisible(bool visible) {
    if (visible) {
        m_converter.setProcessAll(false);
        m_capture_thread.start();
        m_converter_thread.start();
        m_capture.moveToThread(&m_capture_thread);
        m_converter.moveToThread(&m_converter_thread);
        QObject::connect(&m_capture, &Capture::matReady, &m_converter, &Converter::processFrame);
        QObject::connect(&m_converter, &Converter::imageReady, m_image_viewer, &ImageViewer::setImage);
        QMetaObject::invokeMethod(&m_capture, "start");
    }
    setFixedSize(800, 600);
    QDialog::setVisible(visible);
}

void CameraDisplay::reject() {
    QDialog::reject();
}

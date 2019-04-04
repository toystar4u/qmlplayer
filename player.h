#ifndef PLAYER_H
#define PLAYER_H


#include <QQuickItem>

#include <gst/gst.h>


typedef struct _UserData
{
    GstElement *src;
    GstElement *glupload;
    GstElement *fakesink;

    GstElement * pipeline;
    GstElement * sink;
} UserData;


class Player : public QObject
{
    Q_OBJECT
public:
    explicit Player(QObject *parent = nullptr);
    Player(char *uri);
    ~Player();

    void setVideoItem(QQuickItem *videoItem);
    GstElement *pipeline();
    bool playing();

signals:

public slots:
    void pause();
    void resume();
    void toggle();

private:
    UserData m_data;
    bool m_isPlaying=false;

};

#endif // PLAYER_H

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QRunnable>
#include <QObject>

#include <gst/gst.h>

#include <player.h>

class SetPlaying : public QRunnable
{

public:
    SetPlaying(Player *);
    ~SetPlaying();

    void run ();

private:
    Player *m_player=nullptr;
};

SetPlaying::SetPlaying (Player *player)
{
    m_player = player;
}

SetPlaying::~SetPlaying ()
{
}

void
SetPlaying::run ()
{
    m_player->resume();
}



int main(int argc, char *argv[])
{
    int ret;

    if(argc !=2){
        printf("  Usage : ./player [file:///home/root/qt-gstreamer/bbb.avi] or [/dev/video0]");
        return -1;
    }


    // 1) initialize Gstreamer
    gst_init (&argc, &argv);


    QGuiApplication app(argc, argv);

    // 2) create a player based on gstreamer for QML
    Player player(argv[1]);


    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    QQuickItem *videoItem;
    QQuickWindow *rootObject;

    /* find and set the videoItem on the sink */
    rootObject = static_cast<QQuickWindow *> (engine.rootObjects().first());
    videoItem = rootObject->findChild<QQuickItem *> ("videoItem");
    g_assert (videoItem);


    // 3) set videoItem to plyaer
    player.setVideoItem(videoItem);


    // 4) connect signal-slot
    QObject::connect(engine.rootObjects().first(), SIGNAL(playerToggle()),
                        &player, SLOT(toggle()));


    // 5) Run
    rootObject->scheduleRenderJob (new SetPlaying (&player),
                                   QQuickWindow::BeforeSynchronizingStage);

    ret = app.exec();


    // 6) deinitialize Gstreamer
    gst_deinit ();

    return ret;
}

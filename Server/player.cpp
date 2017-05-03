#include "player.h"

QString mpvCommands[] = {"seek", "set_property", "get_property", "observe_property", "playlist-next", "playlist-prev", "loadfile", "loadlist", "playlist-shuffle", "quit"};
QString mpvProperties[] = {"volume", "playback-time", "pause", "ao-mute"};

Player::Player(QString serverName, QObject * parent) : QThread(parent), mpv(new QLocalSocket)
{
    this->player = serverName;
}

Player::~Player()
{
    delete this->mpv;
}

void Player::run()
{
    this->running = true;

    QStringList toRun;

    toRun << "--idle" << "--quiet" << "--input-ipc-server=/tmp/mpvplayer";

    QProcess instance;

    instance.start("mpv", toRun);
    instance.waitForFinished(-1);

    std::cout << "MPV finished" << std::endl;

    this->running = false;
}

void Player::setRunning(bool value)
{
    this->running = value;
}

void Player::listener()
{
    while(this->running)
    {
        QDataStream stream(this->mpv);

        if(!stream.atEnd())
        {
            QString data = QString(stream.device()->readLine());

            QByteArray bytes = data.toUtf8();
            QJsonDocument document = QJsonDocument::fromJson(bytes);
            QJsonObject object = document.object();

            //qDebug() << "Signal emit" + object["error"].toString();

            if(object.contains("event") && object["event"].toString().compare("property-change"))
            {
                qDebug() << "data: " + object["event"].toString() + object["name"].toString();
                emit observer(object["id"].toInt(), object["data"]);
            }
            else if(object["error"].toString().compare("success") == 0)
            {
                emit response(object);
            }
        }
    }
}

void Player::observeProperty(int property)
{
    QJsonArray parameters;

    parameters.append(property);
    parameters.append(mpvProperties[property]);

    this->sendCommand(kObserveProperty, parameters);
}

void Player::sendCommand(kCommand command, QJsonArray parameters)
{
    if(!this->mpv->isOpen())
    {
        this->mpv->connectToServer("/tmp/mpvplayer");
        this->listening = QtConcurrent::run(this, &Player::listener);
    }

    QJsonObject cmd;

    parameters.insert(0, mpvCommands[command]);

    cmd.insert("command", parameters);

    QByteArray bytes = QJsonDocument(cmd).toJson(QJsonDocument::Compact) + "\n";

    if(this->mpv != NULL)
    {
        this->mpv->write(bytes.data(), bytes.length());
        this->mpv->flush();

        std::cout << "Socket command sent." << std::endl;
    }
}

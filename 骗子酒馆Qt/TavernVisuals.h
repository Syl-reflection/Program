#pragma once
// Self-contained, deterministic artwork. No game state or random generator access.
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QEnterEvent>
#include <cmath>
namespace TavernVisuals {
inline void crest(QPainter &p, QRectF r, QColor ink) {
 p.save(); p.translate(r.center()); p.scale(r.width()/100.,r.height()/100.);
 p.setPen(QPen(ink,1.8)); p.setBrush(Qt::NoBrush);
 p.drawEllipse(QRectF(-45,-45,90,90)); p.drawEllipse(QRectF(-40,-40,80,80));
 QPainterPath head; head.moveTo(-28,-18); head.lineTo(-33,-34); head.quadTo(-18,-40,-13,-27);
 head.quadTo(0,-33,13,-27); head.quadTo(18,-40,33,-34); head.lineTo(28,-18);
 head.quadTo(40,3,24,24); head.quadTo(0,44,-24,24); head.quadTo(-40,3,-28,-18);
 p.drawPath(head);
 p.setBrush(ink);
 for(int d:{-1,1}) {QPolygonF stripe; stripe<<QPointF(d*4,-24)<<QPointF(d*21,-19)<<QPointF(d*7,-15);p.drawPolygon(stripe);
 QPolygonF eye; eye<<QPointF(d*7,-4)<<QPointF(d*26,-9)<<QPointF(d*17,2);p.drawPolygon(eye);
 for(int j=0;j<3;++j){QPolygonF side;side<<QPointF(d*31,2+j*7)<<QPointF(d*19,6+j*6)<<QPointF(d*30,7+j*7);p.drawPolygon(side);}}
 QPolygonF nose; nose<<QPointF(-7,10)<<QPointF(7,10)<<QPointF(0,18);p.drawPolygon(nose);
 p.setBrush(Qt::NoBrush);p.drawLine(0,18,0,27);p.drawLine(0,27,-10,22);p.drawLine(0,27,10,22);
 p.restore();
}
inline QPixmap avatar(int seat, bool finished=false) {
 QPixmap pm(240,240);pm.fill(Qt::transparent);QPainter p(&pm);p.setRenderHint(QPainter::Antialiasing);

 static const QPixmap atlas(QStringLiteral(":/ui/tiger-portraits.png"));
 if(!atlas.isNull()){
   const int tileWidth=atlas.width()/2,tileHeight=atlas.height()/2;
   const QRect source((seat%2)*tileWidth,(seat/2)*tileHeight,tileWidth,tileHeight);
   QPainterPath circle;circle.addEllipse(QRectF(8,8,224,224));p.setClipPath(circle);
   if(finished){
     const QImage gray=atlas.copy(source).toImage().convertToFormat(QImage::Format_Grayscale8);
     p.drawImage(QRectF(8,8,224,224),gray);p.fillRect(QRectF(8,8,224,224),QColor(0,0,0,105));
   }else{p.setRenderHint(QPainter::SmoothPixmapTransform);p.drawPixmap(QRectF(8,8,224,224),atlas,source);}
   p.setClipping(false);p.setBrush(Qt::NoBrush);p.setPen(QPen(finished?QColor("#5b554c"):QColor("#a37c41"),3));
   p.drawEllipse(QRectF(8,8,224,224));p.end();return pm;
 }
 QColor colors[]={QColor("#344331"),QColor("#453047"),QColor("#38434a"),QColor("#61422e")};
 QRadialGradient g(110,90,130);g.setColorAt(0,(finished?QColor("#34312b"):colors[seat]).lighter(155));g.setColorAt(1,QColor("#100c09"));
 p.setPen(QPen(QColor("#967443"),3));p.setBrush(g);p.drawEllipse(QRectF(8,8,224,224));
 crest(p,QRectF(32,38,176,176),(finished?QColor("#777269"):QColor("#c9a36c")));
 // Small hat silhouette gives each heraldic tiger a tavern character.
 p.setBrush((finished?QColor("#34312b"):colors[seat]).darker(140));p.setPen(QPen(QColor("#a68149"),2));
 p.drawRoundedRect(QRectF(77,20+seat*3,86,42),8,8);p.drawEllipse(QRectF(58,52+seat*2,124,13));
 p.end();return pm;
}
inline void back(QPainter &p,QRectF r){
 p.save(); QLinearGradient g(r.topLeft(),r.bottomRight());g.setColorAt(0,QColor("#534534"));g.setColorAt(1,QColor("#211c17"));
 p.setPen(QPen(QColor("#b29870"),1.5));p.setBrush(g);p.drawRoundedRect(r,4,4);
 p.setPen(QPen(QColor("#756349"),1));p.setBrush(Qt::NoBrush);p.drawRoundedRect(r.adjusted(4,4,-4,-4),2,2);
 crest(p,QRectF(r.center().x()-r.width()*.36,r.center().y()-r.width()*.36,r.width()*.72,r.width()*.72),QColor("#98805b"));p.restore();
}
inline QPixmap backs(bool pile=false){
 QPixmap pm(pile?140:190,pile?90:82);pm.fill(Qt::transparent);QPainter p(&pm);p.setRenderHint(QPainter::Antialiasing);
 if(pile){for(int i=3;i>=0;--i){p.save();p.translate(43,12+i*5);p.rotate(16);back(p,QRectF(0,0,58,65));p.restore();}}
 else for(int i=0;i<5;++i){p.save();p.translate(4+i*27,3);p.rotate((i-2)*2.);back(p,QRectF(0,0,51,70));p.restore();}
 return pm;
}
}

namespace TavernVisuals {
class HandCard final : public QPushButton {
    QPoint rest_;
    bool hovering_ = false;
    void lift() { move(rest_ - QPoint(0,isChecked()?18:(hovering_ && isEnabled()?10:0))); update(); }
public:
    explicit HandCard(const QString &caption) : QPushButton(caption) {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
        connect(this,&QPushButton::toggled,this,[this]{lift();});
    }
    void setRestGeometry(const QRect &r){rest_=r.topLeft();resize(r.size());lift();}
protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override {hovering_=true;lift();QPushButton::enterEvent(event);}
#else
    void enterEvent(QEvent *event) override {hovering_=true;lift();QPushButton::enterEvent(event);}
#endif
    void leaveEvent(QEvent *event) override {hovering_=false;lift();QPushButton::leaveEvent(event);}
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        p.scale(width()/140.,height()/202.);
        QRectF r(2,2,136,198);
        QLinearGradient paper(0,0,140,202);paper.setColorAt(0,QColor("#e5cca0"));
        paper.setColorAt(.48,QColor("#d9bd8e"));paper.setColorAt(1,QColor("#b99766"));
        p.setPen(QPen(isChecked()?QColor("#f3cf78"):QColor("#745234"),isChecked()?3.5:2));p.setBrush(paper);p.drawRoundedRect(r,7,7);
        p.setPen(QPen(QColor(92,64,31,65),.6));
        for(int i=0;i<210;++i){const int x=7+(i*47)%124,y=7+(i*71)%186;p.drawLine(x,y,x+((i%5)+1),y+1);}
        p.setPen(QPen(QColor("#9c7a4c"),1));p.setBrush(Qt::NoBrush);p.drawRoundedRect(r.adjusted(6,6,-6,-6),4,4);
        for(QPointF c:{QPointF(13,13),QPointF(127,13),QPointF(13,189),QPointF(127,189)})p.drawEllipse(c,4,4);
        const QString rank=text().section('\n',0,0);
        const bool red=rank=="9"||rank=="J"||rank=="K"||rank=="JOKER";
        p.setPen(red?QColor("#7b3524"):QColor("#2c241a"));
        QFont font("Georgia");font.setPixelSize(rank=="JOKER"?24:45);font.setBold(true);p.setFont(font);
        if(rank=="JOKER") {
            font.setPixelSize(23);p.setFont(font);
            for(int i=0;i<rank.size();++i)p.drawText(QRectF(16,17+i*26,26,28),Qt::AlignCenter,rank.mid(i,1));
        } else p.drawText(QRectF(15,13,114,57),Qt::AlignLeft|Qt::AlignVCenter,rank);
        crest(p,QRectF(29,87,82,82),QColor("#896b43"));
        font.setPixelSize(10);font.setBold(false);p.setFont(font);p.setPen(QColor("#805f39"));
        p.drawText(QRectF(15,174,110,16),Qt::AlignCenter,QStringLiteral("T I G E R  T A V E R N"));
        if(hasFocus()){p.setPen(QPen(QColor("#f8e0a2"),2,Qt::DotLine));p.drawRoundedRect(r.adjusted(3,3,-3,-3),5,5);}
        // Noninteractive cards remain readable; only the outline loses emphasis.
    }
};
}

namespace TavernVisuals {
class TableBackdrop final : public QWidget {
    QPixmap scene_;
public:
    explicit TableBackdrop(QWidget *parent):QWidget(parent),scene_(1600,900){
        setObjectName("tableArtwork");setAttribute(Qt::WA_TransparentForMouseEvents);

        const QPixmap background(QStringLiteral(":/ui/tavern-background.png"));
        if(!background.isNull()){
            scene_=background.scaled(1600,900,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
            QPainter shade(&scene_);shade.fillRect(scene_.rect(),QColor(0,0,0,35));
            return;
        }
        scene_.fill(QColor("#0d0906"));QPainter p(&scene_);p.setRenderHint(QPainter::Antialiasing);
        for(int x=0;x<1600;x+=95){
            QLinearGradient wall(x,0,x+90,0);wall.setColorAt(0,QColor("#090806"));wall.setColorAt(.4,QColor("#24190f"));wall.setColorAt(1,QColor("#100c08"));
            p.fillRect(QRect(x,0,91,900),wall);p.setPen(QColor(109,71,34,24));
            for(int k=0;k<6;++k)p.drawLine(x+13*k,0,x+13*k+7,900);
        }
        auto candle=[&p](qreal x,qreal y){
            QRadialGradient light(x,y,155);light.setColorAt(0,QColor(249,160,47,90));light.setColorAt(.3,QColor(185,89,16,25));light.setColorAt(1,Qt::transparent);
            p.setPen(Qt::NoPen);p.setBrush(light);p.drawEllipse(QPointF(x,y),155,155);
            p.setBrush(QColor("#302012"));p.setPen(QPen(QColor("#77542d"),2));p.drawEllipse(QRectF(x-45,y+46,90,18));
            p.setBrush(QColor("#99703b"));p.drawRect(QRectF(x-8,y+9,16,42));
            QPainterPath flame;flame.moveTo(x,y-14);flame.cubicTo(x-16,y+7,x-5,y+20,x+3,y+12);flame.cubicTo(x+11,y+5,x,y-5,x,y-14);
            p.setPen(Qt::NoPen);p.setBrush(QColor("#f3b44f"));p.drawPath(flame);p.setBrush(QColor("#ffedac"));p.drawEllipse(QRectF(x-3,y,6,11));
        };
        candle(206,37);candle(1400,38);candle(46,650);
        QRectF table(66,78,1470,808);
        for(int i=30;i>0;--i){p.setPen(QPen(QColor(0,0,0,8),i*2));p.setBrush(Qt::NoBrush);p.drawEllipse(table.translated(0,22));}
        QRadialGradient rim(770,320,850);rim.setColorAt(0,QColor("#ab7540"));rim.setColorAt(.6,QColor("#654326"));rim.setColorAt(1,QColor("#20160d"));
        p.setPen(QPen(QColor("#785127"),3));p.setBrush(rim);p.drawEllipse(table);
        const QRectF inner=table.adjusted(18,17,-18,-17);QPainterPath clip;clip.addEllipse(inner);
        p.save();p.setClipPath(clip);
        QRadialGradient wood(790,375,790);wood.setColorAt(0,QColor("#694322"));wood.setColorAt(.5,QColor("#472913"));wood.setColorAt(1,QColor("#201309"));p.fillRect(inner,wood);
        // Long uneven grain strokes and plank seams stay deterministic.
        for(int i=0;i<930;++i){
            const qreal y=88+i*.88;
            p.setPen(QPen(i%4==0?QColor(195,126,61,12):QColor(12,5,1,17),i%7==0?1.2:.6));
            QPainterPath line;line.moveTo(65,y);
            for(int x=65;x<=1540;x+=16){const qreal wave=2.3*std::sin(x*.019+i*.38)+1.6*std::sin(x*.007+i*.16);line.lineTo(x,y+wave);}
            p.drawPath(line);
        }
        for(int y=125;y<900;y+=91){p.setPen(QPen(QColor(9,4,1,90),2));p.drawLine(60,y,1540,y);p.setPen(QColor(169,107,48,24));p.drawLine(60,y+2,1540,y+2);}
        for(QPointF knot:{QPointF(350,285),QPointF(1120,565),QPointF(470,712),QPointF(1210,160)}){
            for(int i=0;i<14;++i){p.setPen(QPen(QColor(16,7,2,26),.8));p.setBrush(Qt::NoBrush);p.drawEllipse(knot,8+i*5,2+i*1.6);}
        }
        QRadialGradient shade(800,415,760);shade.setColorAt(0,Qt::transparent);shade.setColorAt(.65,QColor(0,0,0,8));shade.setColorAt(1,QColor(0,0,0,145));p.fillRect(inner,shade);
        p.restore();p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor("#271a0e"),7));p.drawEllipse(inner);
        p.setPen(QPen(QColor("#a17338"),1.5));p.drawEllipse(table.adjusted(7,6,-7,-6));
        for(int i=0;i<12;++i){qreal a=(i+.5)*6.2831853/12.;QPointF v(801+727*std::cos(a),482+397*std::sin(a));p.setPen(QPen(QColor("#26190c"),2));p.setBrush(QColor("#8c6634"));p.drawEllipse(v,5,5);p.setPen(QColor("#c09a59"));p.drawLine(v+QPointF(-2,-1),v+QPointF(2,-1));}
        QRadialGradient vignette(800,430,970);vignette.setColorAt(0,Qt::transparent);vignette.setColorAt(.63,Qt::transparent);vignette.setColorAt(1,QColor(0,0,0,175));p.fillRect(QRect(0,0,1600,900),vignette);
    }
protected:
    void paintEvent(QPaintEvent *)override{QPainter p(this);p.setRenderHint(QPainter::SmoothPixmapTransform);p.drawPixmap(rect(),scene_);}
};
}

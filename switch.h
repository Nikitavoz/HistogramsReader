#ifndef SWITCH_H
#define SWITCH_H

#include <QtWidgets>
const QColor OKcolor("#b0d959"), notOKcolor("#ff2c26");

class Switch : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(int offset READ offset WRITE setOffset)
    Q_PROPERTY(QBrush brush READ brush WRITE setBrush)
    Q_PROPERTY(bool SwitchOnClick READ getSwitchOnClick WRITE setSwitchOnClick)
    bool _switch, _switchOnClick, _orientation;
    qreal _opacity;
    int _x, _y, _margin;
    QBrush _thumb, _brush;
    QPropertyAnimation *_anim = nullptr;

public:
    Switch(QWidget* parent = nullptr, const QBrush &brush = OKcolor):
        QAbstractButton(parent),
        _switch(false),
        _orientation(false),
        _opacity(0.000),
        _margin(3),
        _thumb("#ff2c26"),
        _anim(new QPropertyAnimation(this, "offset", this))
    {
        setMinimumHeight(_orientation ? 12 : 18);
        setMinimumWidth(_orientation ? 18 : 12);
        setCheckable(true);
        setBrush(brush);

        connect(this, &Switch::toggled, this, [=](){
            _switch = isChecked();
            _thumb = _switch ? _brush : notOKcolor;
            setOffset(_margin + (_switch == _orientation ? abs(width() - height()) : 0));
        });
    }

    QSize sizeHint() const override { return QSize(height() + 2 * _margin, 2 * (height() + _margin)); }

    QBrush brush() const {return _brush;}

    void setBrush (const QBrush &brsh) {_brush = brsh;}

    int offset() const {return _orientation ?_x :_y;}

    void setOffset(int o) {
        (_orientation ? _x : _y) = o;
        update();
    }

    bool is_switched() const {return _switch;}

    bool getSwitchOnClick() const {return _switchOnClick;}

    void setSwitchOnClick(const bool &flag) {_switchOnClick = flag;}

    void swipe() {
        _switch = !_switch;
        _thumb = _switch ? _brush : notOKcolor;
        if (_switch) {
            _anim->setStartValue(offset());
            setOffset(_orientation ? width() - height() + _margin :  _margin);
            _anim->setEndValue(_orientation ? width() - height() + _margin :  _margin);
        } else {
            _anim->setStartValue(_orientation ? width() - height() + _margin :  _margin);
            setOffset(_orientation ? _margin : height() - width() + _margin);
            _anim->setEndValue(offset());
        }
		_anim->setDuration(200);
        _anim->start();
        if(!this->getSwitchOnClick()) this->setChecked(_switch);
    }

protected:
    void paintEvent(QPaintEvent *e) override {
        QPainter p(this);
        p.setPen(Qt::NoPen);
        p.setBrush(isEnabled() ? (_switch ? brush() : notOKcolor) : Qt::gray);
        p.setOpacity(isEnabled() ? (_switch ? 0.5 : 0.38) : 0.12);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawRoundedRect(QRect(_margin, _margin,
                                width()  - 2 * _margin,
                                height() - 2 * _margin),
                                ((_orientation ? height() : width()) - 2 * _margin)/2,
                                ((_orientation ? height() : width()) - 2 * _margin)/2);
        p.setOpacity(1.0);
        if (isEnabled()) p.setBrush(_thumb);
        p.drawEllipse(QRectF( _orientation ? offset() : _x ,
                              _orientation ? _y : offset(),
                             (_orientation ? height() : width()) - 2 * _margin,
                             (_orientation ? height() : width()) - 2 * _margin));
        e->accept();
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        if (_switchOnClick) {
            if (e->button() & Qt::LeftButton) {
                swipe();
            }
            QAbstractButton::mouseReleaseEvent(e);
            emit switch_changed();
        } else if (e->button() & Qt::LeftButton) {
            emit clicked(isChecked());
        }
    }

    void resizeEvent(QResizeEvent* e) override {
        _orientation = width() > height() ? true : false;
        if(!_orientation) {
            _x = _margin;
            setOffset(_switch ? _margin : height() - width() + _margin );
        } else {
            _y = _margin;
            setOffset(_switch ?  width() - height() + _margin : _margin);
        }
        update();
        e->accept();
    };

    void enterEvent(QEvent *e) override {
        setCursor(Qt::PointingHandCursor);
        QAbstractButton::enterEvent(e);
    }

signals:
    void switch_changed();
};

#endif // SWITCH_H

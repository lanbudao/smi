#ifndef RECTF_H
#define RECTF_H

#include "utils/Size.h"
#include "utils/innermath.h"

NAMESPACE_BEGIN


class PointF
{
public:
    PointF();
    PointF(double xpos, double ypos);

    inline double manhattanLength() const;

    inline bool isNull() const;

    inline double x() const;
    inline double y() const;
    inline void setX(double x);
    inline void setY(double y);

    inline double &rx();
    inline double &ry();

    inline PointF &operator+=(const PointF &p);
    inline PointF &operator-=(const PointF &p);
    inline PointF &operator*=(double c);
    inline PointF &operator/=(double c);

    friend  inline bool operator==(const PointF &, const PointF &);
    friend  inline bool operator!=(const PointF &, const PointF &);
    friend  inline const PointF operator+(const PointF &, const PointF &);
    friend  inline const PointF operator-(const PointF &, const PointF &);
    friend  inline const PointF operator*(double, const PointF &);
    friend  inline const PointF operator*(const PointF &, double);
    friend  inline const PointF operator+(const PointF &);
    friend  inline const PointF operator-(const PointF &);
    friend  inline const PointF operator/(const PointF &, double);

private:
    double xp;
    double yp;
};

inline PointF::PointF() : xp(0), yp(0) { }

inline PointF::PointF(double xpos, double ypos) : xp(xpos), yp(ypos) { }

inline double PointF::manhattanLength() const
{
    return std::abs(x()) + std::abs(y());
}

inline bool PointF::isNull() const
{
    return std::abs(xp) <= 1e-15 && std::abs(yp) <= 1e-15;
}

inline double PointF::x() const
{
    return xp;
}

inline double PointF::y() const
{
    return yp;
}

inline void PointF::setX(double xpos)
{
    xp = xpos;
}

inline void PointF::setY(double ypos)
{
    yp = ypos;
}

inline double &PointF::rx()
{
    return xp;
}

inline double &PointF::ry()
{
    return yp;
}

inline PointF &PointF::operator+=(const PointF &p)
{
    xp += p.xp;
    yp += p.yp;
    return *this;
}

inline PointF &PointF::operator-=(const PointF &p)
{
    xp -= p.xp; yp -= p.yp; return *this;
}

inline PointF &PointF::operator*=(double c)
{
    xp *= c; yp *= c; return *this;
}

inline bool operator==(const PointF &p1, const PointF &p2)
{
    return ((!p1.xp || !p2.xp) ? FuzzyIsNull(p1.xp - p2.xp) : FuzzyCompare(p1.xp, p2.xp))
            && ((!p1.yp || !p2.yp) ? FuzzyIsNull(p1.yp - p2.yp) : FuzzyCompare(p1.yp, p2.yp));
}

inline bool operator!=(const PointF &p1, const PointF &p2)
{
    return !(p1 == p2);
}

inline const PointF operator+(const PointF &p1, const PointF &p2)
{
    return PointF(p1.xp + p2.xp, p1.yp + p2.yp);
}

inline const PointF operator-(const PointF &p1, const PointF &p2)
{
    return PointF(p1.xp - p2.xp, p1.yp - p2.yp);
}

inline const PointF operator*(const PointF &p, double c)
{
    return PointF(p.xp*c, p.yp*c);
}

inline const PointF operator*(double c, const PointF &p)
{
    return PointF(p.xp*c, p.yp*c);
}

inline const PointF operator+(const PointF &p)
{
    return p;
}

inline const PointF operator-(const PointF &p)
{
    return PointF(-p.xp, -p.yp);
}

inline PointF &PointF::operator/=(double divisor)
{
    xp /= divisor;
    yp /= divisor;
    return *this;
}

inline const PointF operator/(const PointF &p, double divisor)
{
    return PointF(p.xp / divisor, p.yp / divisor);
}

class RectF
{
public:
    RectF(): xp(0.), yp(0.), w(0.), h(0.) {}
    RectF(const PointF &topleft, const SizeF &size) ;
    RectF(const PointF &topleft, const PointF &bottomRight) ;
    RectF(double left, double top, double width, double height) ;

    inline bool isNull() const ;
    inline bool isEmpty() const ;
    inline bool isValid() const ;
    RectF normalized() const ;

    inline double left() const  { return xp; }
    inline double top() const  { return yp; }
    inline double right() const  { return xp + w; }
    inline double bottom() const  { return yp + h; }

    inline double x() const ;
    inline double y() const ;
    inline void setLeft(double pos) ;
    inline void setTop(double pos) ;
    inline void setRight(double pos) ;
    inline void setBottom(double pos) ;
    inline void setX(double pos)  { setLeft(pos); }
    inline void setY(double pos)  { setTop(pos); }

    inline PointF topLeft() const  { return PointF(xp, yp); }
    inline PointF bottomRight() const  { return PointF(xp + w, yp + h); }
    inline PointF topRight() const  { return PointF(xp + w, yp); }
    inline PointF bottomLeft() const  { return PointF(xp, yp + h); }
    inline PointF center() const ;

    inline void setTopLeft(const PointF &p) ;
    inline void setBottomRight(const PointF &p) ;
    inline void setTopRight(const PointF &p) ;
    inline void setBottomLeft(const PointF &p) ;

    inline void moveLeft(double pos) ;
    inline void moveTop(double pos) ;
    inline void moveRight(double pos) ;
    inline void moveBottom(double pos) ;
    inline void moveTopLeft(const PointF &p) ;
    inline void moveBottomRight(const PointF &p) ;
    inline void moveTopRight(const PointF &p) ;
    inline void moveBottomLeft(const PointF &p) ;
    inline void moveCenter(const PointF &p) ;

    inline void translate(double dx, double dy) ;
    inline void translate(const PointF &p) ;

    inline RectF translated(double dx, double dy) const ;
    inline RectF translated(const PointF &p) const ;

    inline RectF transposed() const ;

    inline void moveTo(double x, double y) ;
    inline void moveTo(const PointF &p) ;

    inline void setRect(double x, double y, double w, double h) ;
    inline void getRect(double *x, double *y, double *w, double *h) const;

    inline void setCoords(double x1, double y1, double x2, double y2) ;
    inline void getCoords(double *x1, double *y1, double *x2, double *y2) const;

    inline void adjust(double x1, double y1, double x2, double y2) ;
    inline RectF adjusted(double x1, double y1, double x2, double y2) const ;

    inline SizeF size() const ;
    inline double width() const ;
    inline double height() const ;
    inline void setWidth(double w) ;
    inline void setHeight(double h) ;
    inline void setSize(const SizeF &s) ;

    RectF operator|(const RectF &r) const ;
    RectF operator&(const RectF &r) const ;
    inline RectF& operator|=(const RectF &r) ;
    inline RectF& operator&=(const RectF &r) ;
    friend inline bool operator==(const RectF &, const RectF &);
    friend inline bool operator!=(const RectF &, const RectF &);

private:
    double xp;
    double yp;
    double w;
    double h;
};

inline RectF::RectF(double aleft, double atop, double awidth, double aheight)
    : xp(aleft), yp(atop), w(awidth), h(aheight)
{
}

inline RectF::RectF(const PointF &atopLeft, const SizeF &asize)
    : xp(atopLeft.x()), yp(atopLeft.y()), w(asize.width), h(asize.height)
{
}


inline RectF::RectF(const PointF &atopLeft, const PointF &abottomRight)
    : xp(atopLeft.x()), yp(atopLeft.y()), w(abottomRight.x() - atopLeft.x()), h(abottomRight.y() - atopLeft.y())
{
}

inline bool RectF::isNull() const
{
    return w == 0. && h == 0.;
}

inline bool RectF::isEmpty() const
{
    return w <= 0. || h <= 0.;
}

inline bool RectF::isValid() const
{
    return w > 0. && h > 0.;
}

inline double RectF::x() const
{
    return xp;
}

inline double RectF::y() const
{
    return yp;
}

inline void RectF::setLeft(double pos)
{
    double diff = pos - xp; xp += diff; w -= diff;
}

inline void RectF::setRight(double pos)
{
    w = pos - xp;
}

inline void RectF::setTop(double pos)
{
    double diff = pos - yp; yp += diff; h -= diff;
}

inline void RectF::setBottom(double pos)
{
    h = pos - yp;
}

inline void RectF::setTopLeft(const PointF &p)
{
    setLeft(p.x()); setTop(p.y());
}

inline void RectF::setTopRight(const PointF &p)
{
    setRight(p.x()); setTop(p.y());
}

inline void RectF::setBottomLeft(const PointF &p)
{
    setLeft(p.x()); setBottom(p.y());
}

inline void RectF::setBottomRight(const PointF &p)
{
    setRight(p.x()); setBottom(p.y());
}

inline PointF RectF::center() const
{
    return PointF(xp + w / 2, yp + h / 2);
}

inline void RectF::moveLeft(double pos)
{
    xp = pos;
}

inline void RectF::moveTop(double pos)
{
    yp = pos;
}

inline void RectF::moveRight(double pos)
{
    xp = pos - w;
}

inline void RectF::moveBottom(double pos)
{
    yp = pos - h;
}

inline void RectF::moveTopLeft(const PointF &p)
{
    moveLeft(p.x()); moveTop(p.y());
}

inline void RectF::moveTopRight(const PointF &p)
{
    moveRight(p.x()); moveTop(p.y());
}

inline void RectF::moveBottomLeft(const PointF &p)
{
    moveLeft(p.x()); moveBottom(p.y());
}

inline void RectF::moveBottomRight(const PointF &p)
{
    moveRight(p.x()); moveBottom(p.y());
}

inline void RectF::moveCenter(const PointF &p)
{
    xp = p.x() - w / 2; yp = p.y() - h / 2;
}

inline double RectF::width() const
{
    return w;
}

inline double RectF::height() const
{
    return h;
}

inline SizeF RectF::size() const
{
    return SizeF(w, h);
}

inline void RectF::translate(double dx, double dy)
{
    xp += dx;
    yp += dy;
}

inline void RectF::translate(const PointF &p)
{
    xp += p.x();
    yp += p.y();
}

inline void RectF::moveTo(double ax, double ay)
{
    xp = ax;
    yp = ay;
}

inline void RectF::moveTo(const PointF &p)
{
    xp = p.x();
    yp = p.y();
}

inline RectF RectF::translated(double dx, double dy) const
{
    return RectF(xp + dx, yp + dy, w, h);
}

inline RectF RectF::translated(const PointF &p) const
{
    return RectF(xp + p.x(), yp + p.y(), w, h);
}

inline RectF RectF::transposed() const
{
    return RectF(topLeft(), size().transposed());
}

inline void RectF::getRect(double *ax, double *ay, double *aaw, double *aah) const
{
    *ax = this->xp;
    *ay = this->yp;
    *aaw = this->w;
    *aah = this->h;
}

inline void RectF::setRect(double ax, double ay, double aaw, double aah)
{
    this->xp = ax;
    this->yp = ay;
    this->w = aaw;
    this->h = aah;
}

inline void RectF::getCoords(double *xp1, double *yp1, double *xp2, double *yp2) const
{
    *xp1 = xp;
    *yp1 = yp;
    *xp2 = xp + w;
    *yp2 = yp + h;
}

inline void RectF::setCoords(double xp1, double yp1, double xp2, double yp2)
{
    xp = xp1;
    yp = yp1;
    w = xp2 - xp1;
    h = yp2 - yp1;
}

inline void RectF::adjust(double xp1, double yp1, double xp2, double yp2)
{
    xp += xp1; yp += yp1; w += xp2 - xp1; h += yp2 - yp1;
}

inline RectF RectF::adjusted(double xp1, double yp1, double xp2, double yp2) const
{
    return RectF(xp + xp1, yp + yp1, w + xp2 - xp1, h + yp2 - yp1);
}

inline void RectF::setWidth(double aw)
{
    this->w = aw;
}

inline void RectF::setHeight(double ah)
{
    this->h = ah;
}

inline void RectF::setSize(const SizeF &s)
{
    w = s.width;
    h = s.height;
}

inline RectF& RectF::operator|=(const RectF &r)
{
    *this = *this | r;
    return *this;
}

inline RectF& RectF::operator&=(const RectF &r)
{
    *this = *this & r;
    return *this;
}

inline bool operator==(const RectF &r1, const RectF &r2)
{
    return FuzzyCompare(r1.xp, r2.xp) && FuzzyCompare(r1.yp, r2.yp)
            && FuzzyCompare(r1.w, r2.w) && FuzzyCompare(r1.h, r2.h);
}

inline bool operator!=(const RectF &r1, const RectF &r2)
{
    return !FuzzyCompare(r1.xp, r2.xp) || !FuzzyCompare(r1.yp, r2.yp)
            || !FuzzyCompare(r1.w, r2.w) || !FuzzyCompare(r1.h, r2.h);
}

NAMESPACE_END

#endif // 

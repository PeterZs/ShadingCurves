#ifndef GLSCENE_H
#define GLSCENE_H

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsProxyWidget>
#include <QGraphicsItemGroup>
#include <QTime>
#include <QLabel>
#include "../Utilities/ImageUtils.h"
#include "../Curve/BSplineGroup.h"

// HENRIK: had to rename it since Windows already has a definition of Ellipse
typedef struct Ellipse_b
{
    QPointF center;
    float size;
    QBrush brush;

    Ellipse_b(){}
    Ellipse_b(QPointF _center, float _size, QBrush _brush)
    {
        center = _center;
        size = _size;
        brush = _brush;
    }
} Ellipse_b;

class GLScene : public QGraphicsScene
{
    Q_OBJECT
public:
    typedef enum SketchMode
    {
        ADD_CURVE_MODE,
        IDLE_MODE
    } SketchMode;


    explicit GLScene(QObject *parent = 0);
    virtual ~GLScene();

    //IO functions
    void resetImage();
    bool openImage(std::string fname);
    void saveImage(std::string fname);
    bool openCurves(std::string fname);
    void saveCurves(std::string fname);
    void saveOff(std::string fname);

    //Sketching functions
    void createBSpline();
    void setSurfaceWidth(float _surface_width);
    void recomputeAllSurfaces();
    int registerPointAtScenePos(QPointF scenePos);
    QPointF sceneToImageCoords(QPointF scenePos);
    QPointF imageToSceneCoords(QPointF imgPos);
    bool pick(const QPoint& _mousePos, unsigned int &_nodeIdx,
               unsigned int& _targetIdx, QPointF* _hitPointPtr = NULL);

    //Drawing functions
    void display(bool only_show_splines = false);
    void draw_image(cv::Mat &image);
    void draw_control_point(int point_id);
    void draw_spline(int spline_id, bool only_show_splines = false, bool transform = true);
    void draw_surface(int surface_id);
    void draw_ellipse(QPointF center, float size, QBrush brush);

    void changeResolution(int resWidth, int resHeight);
    void adjustDisplayedImageSize();

    unsigned int getImageHeight() {return m_curImage.cols;}
    unsigned int getImageWidth()  {return m_curImage.rows;}

    ControlPoint& controlPoint(int ref) { return m_splineGroup.controlPoint(ref); }
    BSpline& spline(int ref) { return m_splineGroup.spline(ref); }
    Surface& surface(int ref) { return m_splineGroup.surface(ref); }
    int& curSplineRef() { return m_curSplineIdx; }

    int num_cpts() { return m_splineGroup.num_controlPoints(); }
    int num_splines() { return m_splineGroup.num_splines(); }
    int num_surfaces() { return m_splineGroup.num_surfaces(); }

    bool writeCurrentSurface(std::ostream &ofs, int k=0)
    {
        int splineRef = curSplineRef();
        if (splineRef >= 0 && k < spline(splineRef).num_surfaces())
        {
            spline(splineRef).surfaceAt(k).writeOFF(ofs);
            return true;
        }
        return false;
    }

    std::vector<std::string> OFFSurfaces()
    {
        update_region_coloring();
        std::vector<std::string> surface_strings(num_surfaces());
        for (int i=0; i< num_surfaces(); ++i)
        {
            BSpline& bspline = spline(surface(i).splineRef);
            QPointF normal  = bspline.inward_normal(1);
            if (surface(i).direction == OUTWARD_DIRECTION)
            {
                normal = -normal;
            }
            QPointF pixelPoint = (QPointF)bspline.getPoints()[1] + 5*normal;
            cv::Vec3b color = currentImage().at<cv::Vec3b>(pixelPoint.y(), pixelPoint.x());
            surface_strings[i] =  surface(i).surfaceToOFF(color);

            qDebug("%s", surface_strings[i].c_str());
        }
        return surface_strings;
    }

    cv::Mat curvesImage(bool only_closed_curves = false);
    void update_region_coloring();

    cv::Mat& currentImage()
    {
        return m_curImage;
    }

    cv::Mat& targetImage()
    {
        return m_targetImage;
    }

    SketchMode& sketchmode()
    {
        return m_sketchmode;
    }

    void setImage(cv::Mat im)
    {
        m_curImage = im;
        update();
    }

    cv::Mat* getImage()
    {
        return &m_curImage;
    }

    cv::Mat* displayImage()
    {
        if (curDisplayMode == 1)
        {
            if (m_targetImage.cols>0)
            {
                return &m_targetImage;
            }
            else
                curDisplayMode = (curDisplayMode+1)%3;
            changeDisplayModeText();
        }

        if (curDisplayMode == 2)
        {
            if (surfaceImg.cols>0)
            {
                return &surfaceImg;
            }
            else
                curDisplayMode = (curDisplayMode+1)%3;
            changeDisplayModeText();
        }

        return &m_curImage;
    }

    void changeDisplayModeText()
    {
        if (curDisplayMode == 0)
            displayModeLabel->setText("Blank Image");
        else if (curDisplayMode == 1)
            displayModeLabel->setText("Target Image");
        else
            displayModeLabel->setText("Surface Image");
    }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);

signals:
    void setStatusMessage(QString message);
    void bspline_parameters_changed(bool enabled, float extent, bool _is_slope, bool _has_uniform_subdivision, bool _has_inward, bool _has_outward);

public slots:
    void currentSplineChanged();
    void change_bspline_parameters(float extent, bool _is_slope, bool _has_uniform_subdivision, bool _has_inward, bool _has_outward);

    void delete_all();
    void subdivide_current_spline();
    void toggleShowCurrentCurvePoints(bool status);


private:
    cv::Mat m_curImage;
    cv::Mat m_targetImage;
    int m_curSplineIdx;
    bool hasMoved;
    int curDisplayMode; //0 for blank image, 1 for target image, 2 for surface image

    SketchMode m_sketchmode;
    double  m_modelview [16];
    double  m_projection [16];
    QList<std::pair<uint, uint> > selectedObjects;

    float m_scale;
    QPointF m_translation;
    bool inPanMode;

    QVector<Ellipse_b> ellipses;
    QGraphicsItemGroup *ellipseGroup;

public:
    BSplineGroup m_splineGroup;
    float pointSize;
    bool showControlMesh;
    bool showControlPoints;
    bool showCurrentCurvePoints;
    bool showCurves;
    bool brush;
    int brushType;
    float brushSize;
    bool freehand;
    cv::Mat surfaceImg;
    bool discreteB;

    QSizeF imSize;
    QLabel *displayModeLabel;

};

void nurbsError(uint errorCode);

#endif // GLSCENE_H

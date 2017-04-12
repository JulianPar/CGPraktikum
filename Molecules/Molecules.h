#ifndef WRAPPEDTUBES_H
#define WRAPPEDTUBES_H

#include <QMainWindow>
#include <QGridLayout>
#include <QFrame>
#include <QtCore>
#include <QLocale>
#include <QInputDialog>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
//#include <assimp/scene.h>
//#include <OVR_CAPI_GL.h>

struct atom{
    int index;
    std::string name;
    QVector3D pos;
    float radius;
    std::vector<std::pair<int,std::string>> bonds;
    QVector3D color;

};

struct residue{
    std::string rname;
    std::vector<atom> elements;
};

struct molecule{
    std::string name;
    std::vector<residue> parts;
    std::vector<QVector3D> positions;
};
struct structure{
    std::vector<molecule> mols;
};


class MyGLWidget;

class CGMainWindow : public QMainWindow {
    Q_OBJECT

public slots:

    void loadHin();
    void setOrthogonal();
    void setPerspective();
    void setStereo();
    void setBallStick();
    void setTubes();
    void setLine();


public:

    CGMainWindow (QWidget* parent = 0);
    ~CGMainWindow ();
    MyGLWidget *ogl;
};

class MyGLWidget : public QOpenGLWidget, public QOpenGLFunctions {
    Q_OBJECT

public:
    //const aiScene* scene = NULL;
    MyGLWidget(CGMainWindow*,QWidget*);
    void initShaders();
    void initMaterials();
    void initializeGL();
    //void initializeOVR();
    void refineSolidSphere(const std::vector<QVector3D>&,std::vector<QVector3D>&); 
    void initSolidSphereVBO();
    void initCylinderVBO();
    void initLineVBO(QVector3D c, QVector3D d);
    void initTrianglesVBO(const std::vector<QVector3D>&);
    void initSmoothTrianglesVBO(const std::vector<QVector3D>&);
    void updateBoundingBox(const std::vector<QVector3D>&);
    void pickLine(int,int,QVector3D&,QVector3D&);
    void createLinesVBO(molecule mol);
    double intersectTriangle(const QVector3D&,const QVector3D&,const QVector3D&,const QVector3D&,const QVector3D&);
    double intersectTriangleSets(const QVector3D&,const QVector3D&,int&,int&);
    //void recursive_render(const aiScene*,const aiNode*,QMatrix4x4);
    QVector2D worldCoord(int,int);
    QVector3D mouseToTrackball(int,int);
    // QQuaternion trackball(const QVector3D&,const QVector3D&);
    QMatrix4x4 trackball(const QVector3D&, const QVector3D&);

    float maxLen,zoom;
    float material[19][11];
    QVector3D center,bbMin,bbMax;
    // QQuaternion qNow;
    QMatrix4x4 RNow;

    //ovrHmdDesc hmdDesc;
    //ovrTextureSwapChain textureChain[2];
    //ovrMirrorTexture mirrorTexture;
    GLuint depthId[2];
    GLuint fboId[2];
    GLuint mirrorId, mirrorFBO;
    //ovrSizei mirrorSize; // The size of the texture containing a mirror view of the HMD display
    //ovrSizei idealTextureSize;
    long long frameIndex = 0;

    std::vector<atom> atoms;
    structure struc;
    bool flag;

    int viewMode = 1, representation=1;
    std::vector<std::vector<QVector3D> > triangleSet;
    std::vector<GLuint> vboTriangleSetId;
    GLuint vboSolidSphereId, vboCylinderId,vboLineId,molLinesId;
    int vboSolidSphereSize, vboCylinderSize,vboLineSize;
    std::vector<QVector3D> lineVBO;
    std::vector<QVector3D> molLines;

protected:

    void paintGL();
    void resizeGL(int,int);

    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    void drawLine();
    void drawSolidSphere(const QVector3D&,float,const QVector3D);
    void drawCylinder(const QVector3D&,const QVector3D&,float,const QVector3D);
    void drawCylinderT(const QVector3D&,const QVector3D&,float,const QVector3D);
    void drawTriangleSets();
    void drawMoleculeBS(molecule);
    void drawMoleculeT(molecule);

    CGMainWindow *main;
    int mouseX,mouseY,button;
    // float phi = 30.0, theta = 10.0;

private:
    //ovrSession session;
    QOpenGLShaderProgram p_Diffuse;
    QOpenGLShaderProgram p_Phong;
    QOpenGLShaderProgram p_Lines;
    QMatrix4x4 projection,modelView;
};

#endif

#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVector3D>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>

#include "Molecules.h"



CGMainWindow::CGMainWindow (QWidget* parent) : QMainWindow (parent) {
    resize (720,720);

    QMenu *file = new QMenu("&File",this);
    file->addAction ("Load Hin data", this, SLOT(loadHin()), Qt::CTRL+Qt::Key_L);
    file->addAction ("Quit", qApp, SLOT(quit()), Qt::CTRL+Qt::Key_Q);

    menuBar()->addMenu(file);

    QMenu *view = new QMenu("&View",this);
    view->addAction("Orthogonal", this, SLOT(setOrthogonal()), Qt::CTRL+Qt::Key_O);
    view->addAction("Perspective", this, SLOT(setPerspective()), Qt::CTRL+Qt::Key_P);
    view->addAction("Stereo", this, SLOT(setStereo()), Qt::CTRL+Qt::Key_S);

    menuBar()->addMenu(view);
    QMenu *rep = new QMenu("&Representation",this);
    rep->addAction("Ball and Stick", this, SLOT(setBallStick()), Qt::CTRL+Qt::Key_O);
    rep->addAction("Stick", this, SLOT(setTubes()), Qt::CTRL+Qt::Key_P);
    rep->addAction("Line", this, SLOT(setLine()), Qt::CTRL+Qt::Key_S);

    menuBar()->addMenu(rep);

    QFrame* f = new QFrame (this);
    f->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    f->setLineWidth(2);

    ogl = new MyGLWidget (this,f);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(ogl);
    layout->setMargin(0);
    f->setLayout(layout);

    setCentralWidget(f);

    statusBar()->showMessage("Ready",1000);
}

CGMainWindow::~CGMainWindow () {}

void CGMainWindow::setOrthogonal() {
    ogl->viewMode = 1;
    ogl->update();
}

void CGMainWindow::setPerspective() {
    ogl->viewMode = 2;
    ogl->update();
}

void CGMainWindow::setStereo() {
    ogl->viewMode = 3;
    ogl->update();
}
void CGMainWindow::setBallStick() {
    ogl->representation = 1;
    ogl->update();
}

void CGMainWindow::setTubes() {
    ogl->representation = 2;
    ogl->update();
}
void CGMainWindow::setLine() {
    ogl->representation = 3;
    ogl->update();
}


MyGLWidget::MyGLWidget(CGMainWindow *mainwindow,QWidget* parent ) : QOpenGLWidget (parent) {
    main = mainwindow;
}




void MyGLWidget::updateBoundingBox(const std::vector<QVector3D>& vertices) {
    for(int i=0;i<(int) vertices.size();i++) {
        for(int j=0;j<3;j++) {
            if (vertices[i][j] < bbMin[j]) bbMin[j] = vertices[i][j];
            if (vertices[i][j] > bbMax[j]) bbMax[j] = vertices[i][j];
        }
    }
    center = (bbMax+bbMin)/2;
    maxLen = bbMax[0]-bbMin[0];
    if (bbMax[1]-bbMin[1] > maxLen) maxLen = bbMax[1]-bbMin[1];
    if (bbMax[2]-bbMin[2] > maxLen) maxLen = bbMax[2]-bbMin[2];

//  std::cout << "center = " << center[0] << "," << center[1] << "," << center[2] << std::endl;
//  std::cout << "bbMin  = " << bbMin[0] << "," << bbMin[1] << "," << bbMin[2] << std::endl;
//  std::cout << "bbMax  = " << bbMax[0] << "," << bbMax[1] << "," << bbMax[2] << std::endl;

    zoom = 1.5/maxLen;
}

void MyGLWidget::refineSolidSphere(const std::vector<QVector3D>& sphere,std::vector<QVector3D>& refined) {
    for(size_t i=0;i<sphere.size()/3;i++) {
        const QVector3D& a = sphere[3*i+0];
        const QVector3D& b = sphere[3*i+1];
        const QVector3D& c = sphere[3*i+2];

        QVector3D ab = a+b;
        QVector3D bc = b+c;
        QVector3D ca = c+a;

        ab.normalize();
        bc.normalize();
        ca.normalize();

        refined.push_back(a);
        refined.push_back(ab);
        refined.push_back(ca);

        refined.push_back(ab);
        refined.push_back(b);
        refined.push_back(bc);

        refined.push_back(bc);
        refined.push_back(c);
        refined.push_back(ca);

        refined.push_back(ab);
        refined.push_back(bc);
        refined.push_back(ca);
    }
}

void MyGLWidget::initSolidSphereVBO() {
    std::vector<QVector3D> ico;
    float gr = 0.5*(1.0+sqrt(5.0));

    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(1.0,0.0,gr));
    ico.push_back( QVector3D(gr,-1.0,0.0));

    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(gr,-1.0,0.0));
    ico.push_back( QVector3D(1.0,0.0,-gr));

    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(0.0,gr,-1.0));
    ico.push_back( QVector3D(0.0,gr,1.0));

    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(0.0,gr,1.0));
    ico.push_back( QVector3D(1.0,0.0,gr));

    ico.push_back( QVector3D(gr,1.0,0.0));
    ico.push_back( QVector3D(1.0,0.0,-gr));
    ico.push_back( QVector3D(0.0,gr,-1.0));

    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(-1.0,0.0,gr));
    ico.push_back( QVector3D(-gr,1.0,0.0));

    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(-gr,1.0,0.0));
    ico.push_back( QVector3D(-1.0,0.0,-gr));

    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(0.0,-gr,-1.0));
    ico.push_back( QVector3D(0.0,-gr,1.0));

    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(0.0,-gr,1.0));
    ico.push_back( QVector3D(-1.0,0.0,gr));

    ico.push_back( QVector3D(-gr,-1.0,0.0));
    ico.push_back( QVector3D(-1.0,0.0,-gr));
    ico.push_back( QVector3D(0.0,-gr,-1.0));

    ico.push_back( QVector3D(1.0,0.0,gr));
    ico.push_back( QVector3D(-1.0,0.0,gr));
    ico.push_back( QVector3D(0.0,-gr,1.0));

    ico.push_back( QVector3D(1.0,0.0,gr));
    ico.push_back( QVector3D(0.0,gr,1.0));
    ico.push_back( QVector3D(-1.0,0.0,gr));

    ico.push_back( QVector3D(0.0,gr,1.0));
    ico.push_back( QVector3D(-gr,1.0,0.0));
    ico.push_back( QVector3D(-1.0,0.0,gr));

    ico.push_back( QVector3D(0.0,gr,1.0));
    ico.push_back( QVector3D(0.0,gr,-1.0));
    ico.push_back( QVector3D(-gr,1.0,0.0));

    ico.push_back( QVector3D(0.0,gr,-1.0));
    ico.push_back( QVector3D(-1.0,0.0,-gr));
    ico.push_back( QVector3D(-gr,1.0,0.0));

    ico.push_back( QVector3D(-1.0,0.0,-gr));
    ico.push_back( QVector3D(0.0,gr,-1.0));
    ico.push_back( QVector3D(1.0,0.0,-gr));

    ico.push_back( QVector3D(-1.0,0.0,-gr));
    ico.push_back( QVector3D(1.0,0.0,-gr));
    ico.push_back( QVector3D(0.0,-gr,-1.0));

    ico.push_back( QVector3D(0.0,-gr,-1.0));
    ico.push_back( QVector3D(1.0,0.0,-gr));
    ico.push_back( QVector3D(gr,-1.0,0.0));

    ico.push_back( QVector3D(0.0,-gr,-1.0));
    ico.push_back( QVector3D(gr,-1.0,0.0));
    ico.push_back( QVector3D(0.0,-gr,1.0));

    ico.push_back( QVector3D(0.0,-gr,1.0));
    ico.push_back( QVector3D(gr,-1.0,0.0));
    ico.push_back( QVector3D(1.0,0.0,gr));

    for(size_t i=0;i<ico.size();i++)
        ico[i].normalize();

    for(int i=0;i<2;i++) {
        std::vector<QVector3D> ico_refined;
        refineSolidSphere(ico,ico_refined);
        ico = ico_refined;
    }

    std::vector<QVector3D> vertexWithNormal;
    GLuint id;
    glGenBuffers(1,&id);

    for(size_t i=0;i<ico.size();i++) {
    vertexWithNormal.push_back(ico[i]);
    vertexWithNormal.push_back(ico[i]);
    }

    glBindBuffer(GL_ARRAY_BUFFER,id);
    glBufferData(GL_ARRAY_BUFFER,vertexWithNormal.size()*sizeof(QVector3D),vertexWithNormal.data(),GL_STATIC_DRAW);

    vboSolidSphereId = id;
    vboSolidSphereSize = static_cast<int>(ico.size());
}
void MyGLWidget::initLineVBO(QVector3D c, QVector3D d){
    glGenBuffers(1,&vboLineId);
    lineVBO.push_back(c);
    lineVBO.push_back(d);

    glBindBuffer(GL_ARRAY_BUFFER,vboLineId);
    glBufferData(GL_ARRAY_BUFFER,lineVBO.size()*sizeof(QVector3D),lineVBO.data(),GL_STATIC_DRAW);


}

void MyGLWidget::initCylinderVBO(){

    std::vector<QVector3D> ico;

    int n=100;

    for(int i=0; i<=n; i++){

        float theta = (float)i* (2*M_PI/n);

        // Vertex
        float vertX = cos(theta);
        float vertY = sin(theta);

        ico.push_back(QVector3D(vertX,vertY,0));
        ico.push_back(QVector3D(vertX,vertY,1));


    }

    std::vector<QVector3D> vertexWithNormal;
    GLuint id;
    glGenBuffers(1,&id);

    for(size_t i=0;i<ico.size();i++) {
        vertexWithNormal.push_back(ico[i]);
        vertexWithNormal.push_back(ico[i]);
    }

    glBindBuffer(GL_ARRAY_BUFFER,id);
    glBufferData(GL_ARRAY_BUFFER,vertexWithNormal.size()*sizeof(QVector3D),vertexWithNormal.data(),GL_STATIC_DRAW);

    vboCylinderId = id;
    vboCylinderSize = static_cast<int>(ico.size());
}





void MyGLWidget::initShaders() {
    setlocale(LC_NUMERIC,"C");

    if (!p_Phong.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/VertexShaderPhong.glsl"))
        close();

    if (!p_Phong.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FragmentShaderPhong.glsl"))
        close();

    if (!p_Phong.link())
        close();

    if (!p_Phong.bind())
        close();

    if (!p_Lines.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertexshaderline.glsl"))
        close();

    if (!p_Lines.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Fragmentshaderline.glsl"))
        close();

    if (!p_Lines.link())
        close();

    if (!p_Lines.bind())
        close();

    setlocale(LC_ALL,"");
}

void MyGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    initShaders();
    initCylinderVBO();
    initSolidSphereVBO();

    glClearColor(0.5,0.5,0.5,1);
    // glClearColor(0,0,0,1);

    glEnable(GL_DEPTH_TEST);

    bbMin[0] = bbMin[1] = bbMin[2] =  std::numeric_limits<double>::infinity();
    bbMax[0] = bbMax[1] = bbMax[2] = -std::numeric_limits<double>::infinity();
    center[0] = center[1] = center[2] = 0.0;
    zoom = 1.0;
}

void MyGLWidget::drawCylinder(const QVector3D& c1, const QVector3D& c2, float r,const QVector3D color) {


    QVector3D d=c1-c2;
    QVector3D a(0,0,1);
    QVector3D rot = QVector3D::crossProduct(d,a);
    float phi = asin(rot.length()/(a.length()*d.length()))*180/M_PI;
    if(QVector3D::dotProduct(d,a)>0)
        phi = 180-phi;

    QMatrix4x4 M(modelView);
    M.translate(c1);
    M.rotate(phi,rot.normalized());
    M.scale(r,r,(d).length());

    //const int t = materialType;
    p_Phong.bind();
    p_Phong.setUniformValue("uMVMat", M);
    p_Phong.setUniformValue("uNMat", M.normalMatrix());

    p_Phong.setUniformValue("uAmbient",color*0.7);
    p_Phong.setUniformValue("uDiffuse",QVector3D(color*0.8));
   // p_Phong.setUniformValue("uSpecular",QVector3D(material[0][6],material[0][7],material[0][8]));
   // p_Phong.setUniformValue("uShininess",material[0][9]);

    glBindBuffer(GL_ARRAY_BUFFER, vboCylinderId);
    int vertexLocation = p_Phong.attributeLocation("a_position");
    p_Phong.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);
    int normalLocation = p_Phong.attributeLocation("a_normal");
    p_Phong.enableAttributeArray(normalLocation);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const void*) sizeof(QVector3D));

    glDrawArrays(GL_TRIANGLE_STRIP,0,vboCylinderSize);
}
void MyGLWidget::drawCylinderT(const QVector3D& c1, const QVector3D& c2, float r,const QVector3D color) {

    //QVector3D mid = (c2-c1)/2;
    QVector3D d=(c1-c2)/2;
    QVector3D a(0,0,1);
    QVector3D rot = QVector3D::crossProduct(d,a);
    float phi = asin(rot.length()/(a.length()*d.length()))*180/M_PI;
    if(QVector3D::dotProduct(d,a)>0)
        phi = 180-phi;

    QMatrix4x4 M(modelView);
    M.translate(c1);
    M.rotate(phi,rot.normalized());
    M.scale(r,r,(d).length());

    //const int t = materialType;
    p_Phong.bind();
    p_Phong.setUniformValue("uMVMat", M);
    p_Phong.setUniformValue("uNMat", M.normalMatrix());

    p_Phong.setUniformValue("uAmbient",color*0.7);
    p_Phong.setUniformValue("uDiffuse",QVector3D(color*0.8));
   // p_Phong.setUniformValue("uSpecular",QVector3D(material[0][6],material[0][7],material[0][8]));
   // p_Phong.setUniformValue("uShininess",material[0][9]);

    glBindBuffer(GL_ARRAY_BUFFER, vboCylinderId);
    int vertexLocation = p_Phong.attributeLocation("a_position");
    p_Phong.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);
    int normalLocation = p_Phong.attributeLocation("a_normal");
    p_Phong.enableAttributeArray(normalLocation);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const void*) sizeof(QVector3D));

    glDrawArrays(GL_TRIANGLE_STRIP,0,vboCylinderSize);
}
void MyGLWidget::createLinesVBO(molecule mol){
    for(int i=0;i<mol.parts.size();i++){
        for(int j=0;j<mol.parts[i].elements.size();j++){
            QVector3D pos1 = mol.parts[i].elements[j].pos;

            for(int k=0;k<mol.parts[i].elements[j].bonds.size();k++){
                int iBond = mol.parts[i].elements[j].bonds[k].first;
                QVector3D col1=mol.parts[i].elements[j].color;
               // std::cout<<iBond<<"\n";
                QVector3D pos2;

                //pos2=mol.positions[(iBond-1)];
                //drawCylinderT(pos1,pos2,0.1,col1);
                //drawCylinderT(pos2,pos1,0.1,QVector3D(0.1,0.1,0.1));
                for(int saruman=0;saruman<mol.parts.size();saruman++){
                    for(int countdoku=0;countdoku<mol.parts[saruman].elements.size();countdoku++){
                            if(mol.parts[saruman].elements[countdoku].index==iBond){
                                pos2 = mol.parts[saruman].elements[countdoku].pos;
                                QVector3D col2= mol.parts[saruman].elements[countdoku].color;
                                QVector3D mid=(pos1-pos2)/2+pos2;
                                molLines.push_back(pos1);
                                molLines.push_back(col1);
                                molLines.push_back(mid);
                                molLines.push_back(col1);
                                molLines.push_back(pos2);
                                molLines.push_back(col2);
                                molLines.push_back(mid);
                                molLines.push_back(col2);
                            }
                        }
                }
            }
        }
  }
    glGenBuffers(1,&molLinesId);
    glBindBuffer(GL_ARRAY_BUFFER,molLinesId);
    glBufferData(GL_ARRAY_BUFFER,molLines.size()*sizeof(QVector3D),molLines.data(),GL_STATIC_DRAW);
}


void MyGLWidget::drawLine(){

    QMatrix4x4 M(modelView);
    QMatrix4x4 P(this->projection);

    p_Lines.bind();
    p_Lines.setUniformValue("uMVMat", M);
    p_Lines.setUniformValue("uPMat", P);
    p_Lines.setUniformValue("uNMat", M.normalMatrix());


    glBindBuffer(GL_ARRAY_BUFFER, molLinesId);
    int vertexLocation = p_Lines.attributeLocation("a_position");
    p_Lines.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(QVector3D), 0);
    int colorLocation = p_Lines.attributeLocation("a_color");
    p_Lines.enableAttributeArray(colorLocation);
    glVertexAttribPointer(colorLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const void*) sizeof(QVector3D));

    glDrawArrays(GL_LINES,0,molLines.size()/2);
}


void MyGLWidget::drawSolidSphere(const QVector3D& c, float r,const QVector3D color) {
    QMatrix4x4 M(modelView);
    M.translate(c);
    M.scale(r);

    //const int t = materialType;
    p_Phong.bind();
    p_Phong.setUniformValue("uMVMat", M);
    p_Phong.setUniformValue("uNMat", M.normalMatrix());

    p_Phong.setUniformValue("uAmbient",color*0.7);
    p_Phong.setUniformValue("uDiffuse",color*0.8);
    //p_Phong.setUniformValue("uSpecular",QVector3D(material[0][6],material[0][7],material[0][8]));
    //p_Phong.setUniformValue("uShininess",material[0][9]);

    glBindBuffer(GL_ARRAY_BUFFER, vboSolidSphereId);
    int vertexLocation = p_Phong.attributeLocation("a_position");
    p_Phong.enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);
    int normalLocation = p_Phong.attributeLocation("a_normal");
    p_Phong.enableAttributeArray(normalLocation);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const void*) sizeof(QVector3D));

    glDrawArrays(GL_TRIANGLES,0,vboSolidSphereSize);
}

void MyGLWidget::drawTriangleSets() {
    //const int t = materialType;
    p_Phong.bind();
    p_Phong.setUniformValue("uPMat", projection);
    p_Phong.setUniformValue("uMVMat", modelView);
    p_Phong.setUniformValue("uNMat", modelView.normalMatrix());
    //p_Phong.setUniformValue("uAmbient",QVector3D(material[t][0],material[t][1],material[t][2]));
    //p_Phong.setUniformValue("uDiffuse",QVector3D(material[t][3],material[t][4],material[t][5]));
    //p_Phong.setUniformValue("uSpecular",QVector3D(material[t][6],material[t][7],material[t][8]));
    //p_Phong.setUniformValue("uShininess",material[t][9]);

    for(int i=0;i<(int) triangleSet.size();i++) {
        glBindBuffer(GL_ARRAY_BUFFER,vboTriangleSetId[i]);

        int vertexLocation = p_Phong.attributeLocation("a_position");
        p_Phong.enableAttributeArray(vertexLocation);
        glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), 0);

        int normalLocation = p_Phong.attributeLocation("a_normal");
        p_Phong.enableAttributeArray(normalLocation);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 2*sizeof(QVector3D), (const GLvoid*) (3*sizeof(float)));

        glDrawArrays(GL_TRIANGLES,0,triangleSet[i].size());
    }
}

void MyGLWidget::drawMoleculeBS(molecule mol){

    for(int i=0;i<mol.parts.size();i++){
        for(int j=0;j<mol.parts[i].elements.size();j++){
            drawSolidSphere(mol.parts.at(i).elements[j].pos,mol.parts.at(i).elements[j].radius/200,mol.parts.at(i).elements[j].color);
        }
    }


    for(int i=0;i<mol.parts.size();i++){
        for(int j=0;j<mol.parts[i].elements.size();j++){
            QVector3D pos1 = mol.parts[i].elements[j].pos;

            for(int k=0;k<mol.parts[i].elements[j].bonds.size();k++){
                std::string bondType=mol.parts[i].elements[j].bonds[k].second;
                int iBond = mol.parts[i].elements[j].bonds[k].first;
               // std::cout<<iBond<<"\n";
                QVector3D pos2;
                pos2=mol.positions[(iBond-1)];
                if((bondType=="s") || (bondType=="a")){
                    drawCylinder(pos1,pos2,0.1,QVector3D(0.1,0.1,0.1));
                }
                if(bondType=="d"){
                    float a1=pos1.x()+0.06;
                    QVector3D pos1a{a1,pos1.y(),pos1.z()};
                    float a2=pos2.x()+0.06;
                    QVector3D pos2a{a2,pos2.y(),pos2.z()};
                    drawCylinder(pos1a,pos2a,0.04,QVector3D(0.1,0.1,0.1));
                    float b1=pos1.x()-0.06;
                    QVector3D pos1b{b1,pos1.y(),pos1.z()};
                    float b2=pos2.x()-0.06;
                    QVector3D pos2b{b2,pos2.y(),pos2.z()};
                    drawCylinder(pos1b,pos2b,0.04,QVector3D(0.1,0.1,0.1));
                }
               /* if(bondType=="t"){
                    float a1=pos1.x()+0.07;
                    float a11=pos1.y()+0.07;
                    QVector3D pos1a{a1,a11,pos1.z()};
                    float a2=pos2.x()+0.07;
                    float a22=pos1.y()+0.07;
                    QVector3D pos2a{a2,a22,pos2.z()};
                    drawCylinder(pos1a,pos2a,0.04,QVector3D(0.1,0.1,0.1));
                    float b1=pos1.x()-0.07;
                    float b11=pos1.y()-0.07;
                    QVector3D pos1b{b1,b11,pos1.z()};
                    float b2=pos2.x()-0.07;
                    float b22=pos2.y()-0.07;
                    QVector3D pos2b{b2,b22,pos2.z()};
                    drawCylinder(pos1b,pos2b,0.04,QVector3D(0.1,0.1,0.1));
                    float b111=pos1.y()+0.09;
                    QVector3D pos1c{b1,b111,pos1.z()};
                    float b222=pos2.y()+0.09;
                    QVector3D pos2c{b2,b222,pos2.z()};
                    drawCylinder(pos1c,pos2c,0.04,QVector3D(0.1,0.1,0.1));
                }*/
               /* for(int saruman=0;saruman<mol.parts.size();saruman++){
                    for(int countdoku=0;countdoku<mol.parts[saruman].elements.size();countdoku++){
                            if(mol.parts[saruman].elements[countdoku].index==iBond){
                                pos2 = mol.parts[saruman].elements[countdoku].pos;
                                //std::cout<<saruman<<"  "<<countdoku<<"  "<<std::endl;
                                drawCylinder(pos1,pos2,0.1,QVector3D(0.1,0.1,0.1));
                            }
                        }
                }*/





            }

        }
    }


}
void MyGLWidget::drawMoleculeT(molecule mol){
    for(int i=0;i<mol.parts.size();i++){
        for(int j=0;j<mol.parts[i].elements.size();j++){
            drawSolidSphere(mol.parts.at(i).elements[j].pos,0.1,mol.parts.at(i).elements[j].color);
        }
    }


    for(int i=0;i<mol.parts.size();i++){
        for(int j=0;j<mol.parts[i].elements.size();j++){
            QVector3D pos1 = mol.parts[i].elements[j].pos;

            for(int k=0;k<mol.parts[i].elements[j].bonds.size();k++){
                int iBond = mol.parts[i].elements[j].bonds[k].first;
                QVector3D col1=mol.parts[i].elements[j].color;
               // std::cout<<iBond<<"\n";
                QVector3D pos2;
                //pos2=mol.positions[(iBond-1)];
                //drawCylinderT(pos1,pos2,0.1,col1);
                //drawCylinderT(pos2,pos1,0.1,QVector3D(0.1,0.1,0.1));
                for(int saruman=0;saruman<mol.parts.size();saruman++){
                    for(int countdoku=0;countdoku<mol.parts[saruman].elements.size();countdoku++){
                            if(mol.parts[saruman].elements[countdoku].index==iBond){
                                pos2 = mol.parts[saruman].elements[countdoku].pos;
                                QVector3D col2= mol.parts[saruman].elements[countdoku].color;
                                //std::cout<<saruman<<"  "<<countdoku<<"  "<<std::endl;
                                drawCylinderT(pos1,pos2,0.1,col1);
                                drawCylinderT(pos2,pos1,0.1,col2);
                            }
                        }
                }
            }
        }
    }
}



double MyGLWidget::intersectTriangle(const QVector3D& p0,const QVector3D& p1,const QVector3D& a,const QVector3D& b,const QVector3D& c) {
    QMatrix4x4 M(p0[0]-p1[0],b[0]-a[0],c[0]-a[0],0.0,
                 p0[1]-p1[1],b[1]-a[1],c[1]-a[1],0.0,
                 p0[2]-p1[2],b[2]-a[2],c[2]-a[2],0.0,
                 0.0,        0.0,      0.0,      1.0);
    double inf = std::numeric_limits<double>::infinity();

    bool invertible;
    QMatrix4x4 M1 = M.inverted(&invertible);
    if (!invertible) return inf;

    QVector4D l = M1 * QVector4D(p0[0]-a[0],p0[1]-a[1],p0[2]-a[2],1.0);

    if (l[0] < 0.0) return inf;
    if (l[1] < 0.0) return inf;
    if (l[2] < 0.0) return inf;
    if (l[1] > 1.0) return inf;
    if (l[2] > 1.0) return inf;
    if (l[1]+l[2] > 1.0) return inf;

    return l[0];
}

double MyGLWidget::intersectTriangleSets(const QVector3D& p0,const QVector3D& p1,int& model,int& triangle) {
    double lmin = std::numeric_limits<double>::infinity();
    int imin = -1, jmin = -1;

    for(int i=0;i<(int) triangleSet.size();i++) {
        for(int j=0;j<(int) triangleSet[i].size();j+=3) {
            const QVector3D& a = triangleSet[i][j+0];
            const QVector3D& b = triangleSet[i][j+1];
            const QVector3D& c = triangleSet[i][j+2];
            double l = intersectTriangle(p0,p1,a,b,c);

            if (l < lmin) {
                lmin = l;
                imin = i;
                jmin = j;
            }
        }
    }

    // std::cout << "imin = " << imin << "  jmin = " << jmin << "  lmin = " << lmin << std::endl;

    return lmin;
}

QVector2D MyGLWidget::worldCoord(int x, int y) {
    double dx,dy;

    if (width() > height()) {
        dx = (2.0*x-width())/height();
        dy = 1.0-2.0*y/(double) height();
    } else {
        dx = 2.0*x/(double) width()-1.0;
        dy = (height()-2.0*y)/width();
    }

    return QVector2D(dx,dy);
}

QVector3D MyGLWidget::mouseToTrackball(int x, int y) {
    QVector2D u = worldCoord(x,y);
    QVector3D v(u[0],u[1],0.0);
    double d = v[0]*v[0]+v[1]*v[1];
    if (d > 1.0)
         v /= sqrt(d);
    else v[2] = sqrt(1.0-d*d);

    return v;
}

/*
QQuaternion MyGLWidget::trackball(const QVector3D& u, const QVector3D& v) {
    QVector3D uxv = QVector3D::crossProduct(u,v);
    double uv = QVector3D::dotProduct(u,v);
    QQuaternion ret(1+uv,uxv);
    ret.normalize();
    return ret;
}
*/

QMatrix4x4 MyGLWidget::trackball(const QVector3D& u, const QVector3D& v) {
    QVector3D uxv = QVector3D::crossProduct(u,v);
    double sphi = uxv.length();
    double cphi = QVector3D::dotProduct(u,v);
    double phi = atan2(sphi,cphi);
    uxv.normalize();
    QMatrix4x4 R;
    R.setToIdentity();
    R.rotate(180.0*phi/M_PI,uxv);
    return R;
}

void MyGLWidget::paintGL() {
    glLineWidth(10.0*zoom);
    modelView = RNow;
//  modelView.rotate(qNow);
//  modelView.rotate(theta,1.0,0.0,0.0);
//  modelView.rotate(phi,0.0,1.0,0.0);
    modelView.scale(15,15,15);
    modelView.scale(zoom,zoom,zoom);
    modelView.translate(-center);

    float mynear = 1;
    float myfar = 1000;
    float aspectratio = width()/(float) height();
    float displayHeight = 30;
    float displayWidth = aspectratio*displayHeight;
    float camX = 130;
    float camY = 32;
    float camZ = 205;
    float left   = mynear*(-displayWidth/2-camX)/camZ;
    float right  = mynear*( displayWidth/2-camX)/camZ;
    float bottom = mynear*(-displayHeight/2-camY)/camZ;
    float top    = mynear*( displayHeight/2-camY)/camZ;
    std::cout<<zoom<<"\n";
    projection.setToIdentity();
    if (viewMode == 1)
        projection.ortho(-displayWidth/2,displayWidth/2,-displayHeight/2,displayHeight/2,mynear,myfar);
    if (viewMode == 2)
        projection.frustum(left,right,bottom,top,mynear,myfar);
    projection.translate(QVector3D(-camX,-camY,-camZ));

    camX = -3;
    left   = mynear*(-displayWidth/2-camX)/camZ;
    right  = mynear*( displayWidth/2-camX)/camZ;
    bottom = mynear*(-displayHeight/2-camY)/camZ;
    top    = mynear*( displayHeight/2-camY)/camZ;

    QMatrix4x4 PLeft;
    PLeft.frustum(left,right,bottom,top,mynear,myfar);
    PLeft.translate(QVector3D(-camX,-camY,-camZ));

    camX = 3;
    left   = mynear*(-displayWidth/2-camX)/camZ;
    right  = mynear*( displayWidth/2-camX)/camZ;
    bottom = mynear*(-displayHeight/2-camY)/camZ;
    top    = mynear*( displayHeight/2-camY)/camZ;

    QMatrix4x4 PRight;
    PRight.frustum(left,right,bottom,top,mynear,myfar);
    PRight.translate(QVector3D(-camX,-camY,-camZ));

    glColorMask(true,true,true,true);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //drawSolidSphere(QVector3D(0,0,0),1);

    if (viewMode == 3) {
        glColorMask(true,false,false,true);
        projection = PLeft;
        drawTriangleSets();

        //glClear(GL_DEPTH_BUFFER_BIT);

        glColorMask(false,false,true,true);
        projection = PRight;
        drawTriangleSets();
    } else {


    for(int h=0;h<struc.mols.size();h++){
        if(representation==1){
            drawMoleculeBS(struc.mols[h]);
        }
        if(representation==2){
            drawMoleculeT(struc.mols[h]);
        }
        if(representation==3){
            drawLine();
        }
    }

/*
    for(int h=0;h<struc.mols.size();h++){
        for(int i=0;i<struc.mols[h].parts.size();i++){
            for(int j=0;j<struc.mols[h].parts[i].elements.size();j++){
                drawSolidSphere(struc.mols[h].parts.at(i).elements[j].pos,struc.mols[h].parts.at(i).elements[j].radius/200,struc.mols[h].parts.at(i).elements[j].color);
            }
        }
    }
    for(int h=0;h<struc.mols.size();h++){
        for(int i=0;i<struc.mols[h].parts.size();i++){
            for(int j=0;j<struc.mols[h].parts[i].elements.size();j++){
                QVector3D pos1 = struc.mols[h].parts[i].elements[j].pos;
                for(int k=0;k<struc.mols[h].parts[i].elements[j].bonds.size();k++){
                    int iBond = struc.mols[h].parts[i].elements[j].bonds[k].first;
                    QVector3D pos2;
                    for(int saruman=0;saruman<struc.mols[h].parts.size();saruman++){
                        for(int countdoku=0;countdoku<struc.mols[h].parts[saruman].elements.size();countdoku++){
                                if(struc.mols[h].parts[saruman].elements[countdoku].index==iBond){
                                    pos2 = struc.mols[h].parts[saruman].elements[countdoku].pos;
                                    //std::cout<<saruman<<"  "<<countdoku<<"  "<<std::endl;
                                    drawCylinder(pos1,pos2,0.1,QVector3D(0.1,0.1,0.1));
                                }
                            }
                    }
                }
            }
        }
      }
    /*
    for(int i=0;i<atoms.size();i++){
        for(int j=i;j<atoms[i].bonds.size();j++){
            int iBond = atoms[i].bonds[j].first;
            QVector3D pos1 = atoms[i].pos;
            QVector3D pos2 = atoms[iBond-1].pos;
            drawCylinder(pos1,pos2,0.1,QVector3D(0.1,0.1,0.1));
        }
    }
*/


    if (struc.mols.size()==0){
        /*drawSolidSphere(QVector3D(1,0,0),0.3,QVector3D(0.2,0.2,0.2));
        drawSolidSphere(QVector3D(0,2,0),0.3,QVector3D(0.2,0.2,0.2));
        drawCylinder(QVector3D(1,0,0),QVector3D(0,2,0),0.1,QVector3D(0.1,0.1,0.1));*/
        drawSolidSphere(QVector3D(1,0,0),0.1,QVector3D(0.2,0.2,0.2));
        drawSolidSphere(QVector3D(0,2,0),0.1,QVector3D(0.2,0.2,0.2));
        //drawCylinderT(QVector3D(1,0,0),QVector3D(0,2,0),0.1,QVector3D(0.1,0.1,0.1));
        //drawCylinderT(QVector3D(0,2,0),QVector3D(1,0,0),0.1,QVector3D(0.7,0.7,0.7));
        drawLine();
        //drawCylinder(QVector3D(0,2,3),QVector3D(1,0,0),0.5,QVector3D(0.1,0.1,0.1));
        //drawSolidSphere(QVector3D(0,3,0),1,QVector3D(0.2,0.2,0.2));
       // drawCylinder(QVector3D(0,3,0),QVector3D(1,0,0),0.5,QVector3D(0.1,0.1,0.1));
    }
    drawTriangleSets();
    }


}

void MyGLWidget::resizeGL(int width, int height) {
    glViewport(0,0,width,height);
    update();
}

void MyGLWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->delta();

    QVector3D u,v;
    pickLine(event->x(),event->y(),u,v);
    int model = -1,triangle = -1;
    double l = intersectTriangleSets(u,v,model,triangle);
    QVector3D vu = v - u ;
    if (triangle < 0) {
        QVector3D cu = center - u;
        l = QVector3D::dotProduct(vu,cu)/QVector3D::dotProduct(vu,vu);
    }
    QVector3D p = u + vu*l;

    double factor = (delta > 0)? 1.1 : 1/1.1;
    zoom *= factor;
    center = p + (center-p)/factor;

    update();
}

void MyGLWidget::pickLine(int x, int y, QVector3D& p0, QVector3D& p1) {
    QVector2D p((2.0*x)/width()-1.0,1.0-(2.0*y)/height());

    QMatrix4x4 PM = projection*modelView;
    QVector4D a = PM.inverted()*QVector4D(p[0],p[1],-1.0,1.0);
    QVector4D b = PM.inverted()*QVector4D(p[0],p[1],1.0,1.0);

    p0 = a.toVector3DAffine();
    p1 = b.toVector3DAffine();
}

void MyGLWidget::mousePressEvent(QMouseEvent *event) {
    button = event->button();
    mouseX = event->x();
    mouseY = event->y();

    QVector3D p0,p1;
    pickLine(mouseX,mouseY,p0,p1);


    update();
}

void MyGLWidget::mouseReleaseEvent(QMouseEvent*) {}

void MyGLWidget::mouseMoveEvent(QMouseEvent* event) {
    if (button == Qt::LeftButton) {
        QVector3D p1 = mouseToTrackball(mouseX,mouseY);
        QVector3D p2 = mouseToTrackball(event->x(),event->y());

//      phi += (event->x()-mouseX)/5.0;
//      theta += (event->y()-mouseY)/5.0;
//      if (theta < -90.0) theta = -90.0;
//      if (theta >  90.0) theta =  90.0;

//      QQuaternion q = trackball(p1,p2);
//      qNow = q * qNow;
//      qNow.normalize();

        RNow = trackball(p1,p2) * RNow;
    }

    if (button == Qt::RightButton) {
        QVector3D u0,u1,v0,v1;
        QVector3D vu,cu,p0,p1;
        pickLine(mouseX,mouseY,u0,v0);
        int model = -1,triangle = -1;
        vu = v0 - u0;
        double l = intersectTriangleSets(u0,v0,model,triangle);
        if (triangle < 0) {
            cu = center - u0;
            l = QVector3D::dotProduct(vu,cu)/QVector3D::dotProduct(vu,vu);
        }
        p0 = u0 + vu*l;

        pickLine(event->x(),event->y(),u1,v1);

        vu = v1 - u1;
        if (triangle < 0)
            cu = center - u1;
        else
            cu = p0 - u1;
        l = QVector3D::dotProduct(vu,cu)/QVector3D::dotProduct(vu,vu);
        p1 = u1 + vu*l;

        center -= p1 - p0;
    }

    mouseX = event->x();
    mouseY = event->y();

    update();
}

int main (int argc, char **argv) {
    QApplication app(argc, argv);

    CGMainWindow w(NULL);

    w.show();

    return app.exec();
}

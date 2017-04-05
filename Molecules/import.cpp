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
#include "Molecules.h";
void CGMainWindow::loadHin(){

    QString fn = QFileDialog::getOpenFileName(this, "Load Ice Cube Data ...", QString(), "HIN files (*.hin)" );
    if (fn.isEmpty()) return;
    std::cout<<"Loading Ice Cube Data"<<std::endl;

    std::ifstream instream(fn.toLatin1(),std::ios::binary);

    std::vector<std::string> lines;
    if(instream.is_open()){
        while(!instream.eof()){
            std::string st;
            getline(instream,st);
            lines.push_back(st);

        }
    }

    int begin = 0;
    int end = 0;
    for(int i = 0; i< lines.size(); i++){
        std::string temp = lines[i];
        std::cout << temp << "\n";
        if(temp.substr(0,3) == "mol"){
            begin = i;
            ogl->m.name = temp;
        }
        if(temp.substr(0,6) == "endmol"){
            end = i;
        }
    }

    std::vector<int> beginr;
    std::vector<int> endr;
    for(int j = begin; j < end; j++){

        std::string temp = lines[j];
        if(temp.substr(0,3)=="res"){
            beginr.push_back(j);

        }
        if(temp.substr(0,6)=="endres"){
            endr.push_back(j);
        }
     }

    for(int dracula=0;dracula<beginr.size();dracula++){
        residue r;
        r.rname=lines[beginr[dracula]];
        std::cout<<r.rname<<"\n";
        for(int fumanchu =beginr[dracula];fumanchu<endr[dracula];fumanchu++){
            std::string temp=lines[fumanchu];
            if(temp.substr(0,4) == "atom"){

               float i,x,y,z;
               std::string name;
               std::istringstream f(temp);
               std::string str;
               int len;
               std::string dummy;
               int counter =0;
               std::vector<std::pair<int,std::string>> seanconnery;
               while (getline(f, str, ' ')) {

                   if(counter==3){
                       std::stringstream sstr;
                       sstr << str;
                       name=str;
                   }
                   if(counter==7){
                       std::stringstream sstr;
                       sstr << str;
                       sstr >> x;
                   }
                   if(counter==8){
                       std::stringstream sstr;
                       sstr << str;
                       sstr >> y;
                   }
                   if(counter==9){
                       std::stringstream sstr;
                       sstr << str;
                       sstr >> z;
                   }
                   /*if(counter==10){
                       std::stringstream sstr;
                       sstr<< str;
                       sstr>>len;
                   }*/
                   if(counter>10&&counter%2==1){
                       std::stringstream sstr;
                       sstr<<str;
                       sstr>>len;
                   }
                   if(counter>10&&counter%2==0){
                       std::stringstream sstr;
                       sstr<<str;
                       sstr>>dummy;
                       seanconnery.push_back({len,dummy});
                       std::cout<<seanconnery[0].first<<"\n";
                   }
                   counter++;

               }

               atom a{name,QVector3D{x,y,z},0.0,seanconnery};
               r.elements.push_back(a);
            }

        }
    ogl->m.parts.push_back(r);
    }

    QString fn2 = QFileDialog::getOpenFileName(this, "Load Ice Cube Data ...", QString(), "txt files (*.txt)" );
    if (fn2.isEmpty()) return;
    std::cout<<"Loading Radius"<<std::endl;

    std::ifstream ist(fn2.toLatin1(),std::ios::binary);
    std::vector<std::tuple<std::string,float,QVector3D>> vec;
    if(ist.is_open()){
       while(!ist.eof()){
           float value;
           std::string key;
           float r,g,b;
           ist>>key>>value;
           ist >>r>>g>>b;
           QVector3D v {r/255,g/255,b/255};
           std::cout<<key<<", "<<value<<"\n";
           std::tuple<std::string,float,QVector3D> ma{key,value,v};
           vec.push_back(ma);
       }
    }
    else{
        std::cout<<"File not found \n";
    }
    std::cout<<"change atom size"<<"\n";
    for(int i=0;i<ogl->m.parts.size();i++){
        for(int j=0;j<ogl->m.parts[i].elements.size();j++){
             //std::cout<<"change atom sizei"<<"\n";
            for(int k=0;k<vec.size();k++){
                std::string nam=ogl->m.parts[i].elements[j].name;
                if(nam==std::get<0>(vec[k])){
                    ogl->m.parts[i].elements[j].radius=std::get<1>(vec[k]);
                    ogl->m.parts[i].elements[j].color=std::get<2>(vec[k]);
                    //std::cout<<ogl->m.parts[i].elements[j].radius;
                    //std::cout<<ogl->m.parts[i].elements[j].color.x()<<", "<<ogl->m.parts[i].elements[j].color.y()<<", "<<ogl->m.parts[i].elements[j].color.z()<<"\n";
                }
            }
        }
    }
    ogl->flag = true;
    ogl->update();
}

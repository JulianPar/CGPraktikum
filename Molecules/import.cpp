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
               int counter =0;

               while (getline(f, str, ' ')) {
                   std::string dummy;
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

                   counter++;

               }

               atom a{name,QVector3D{x,y,z}};
               r.elements.push_back(a);
            }

        }
    ogl->m.parts.push_back(r);
    }
    std::ifstream ist{"C:/Users/AlexP/Desktop/CG/Molecules/atomic_radius.txt"};
    std::vector<std::pair<std::string,float>> vec;
    if(ist.is_open()){
       while(!ist.eof()){
           float value;
           std::string key;
           ist>>key>>value;
           std::cout<<key<<", "<<value<<"\n";
           std::pair<std::string,float> ma{key,value};
           vec.push_back(ma);
       }
    }
    else{
        std::cout<<"File not found \n";
    }
    std::cout<<"change atom size"<<"\n";
    for(int i=0;i<ogl->m.parts.size();i++){
        for(int j=0;j<ogl->m.parts[i].elements.size();j++){
             std::cout<<"change atom sizei"<<"\n";
            for(int k=0;k<vec.size();k++){
                std::string nam=ogl->m.parts[i].elements[j].name;
                if(nam==vec[k].first){
                    ogl->m.parts[i].elements[j].radius=vec[k].second;
                    std::cout<<ogl->m.parts[i].elements[j].radius;
                }
            }
        }
    }
    ogl->flag = true;
    ogl->update();
}

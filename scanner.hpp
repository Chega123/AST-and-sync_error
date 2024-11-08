#ifndef SCANNER_HPP
#define SCANNER_HPP

#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<sstream>
#include<map>
#include "parser.hpp"
using namespace std;

char* text_Arr = nullptr;
int indexx = 0;
int sizee = 0;
int line_global = 1;
int col_global = 0;

vector<token>tokens;

map<string, string> tokens_list = {
    pair<string,string>("bool","TOKEN_BoolType"),
    pair<string,string>("char","TOKEN_CharType"),
    pair<string,string>("array","TOKEN_ARRAY"), //
    pair<string,string>("else","TOKEN_else"),
    pair<string,string>("false","TOKEN_False"),
    pair<string,string>("for","TOKEN_for"),
    pair<string,string>("function","TOKEN_FUNCTION"), //
    pair<string,string>("if","TOKEN_if"),
    pair<string,string>("int","TOKEN_IntType"),
    pair<string,string>("print","TOKEN_print"),
    pair<string,string>("return","TOKEN_return"),
    pair<string,string>("string","TOKEN_StringType"),
    pair<string,string>("true","TOKEN_True"),
    pair<string,string>("void","TOKEN_VoidType"),
    pair<string,string>("while","TOKEN_WHILE"), //

    pair<string,string>("[","TOKEN_["),
    pair<string,string>("]","TOKEN_]"),
    pair<string,string>("(","TOKEN_("),
    pair<string,string>(")","TOKEN_)"),
    pair<string,string>("++","TOKEN_++"), //
    pair<string,string>("--","TOKEN_--"), //
    pair<string,string>("-","TOKEN_-"),
    pair<string,string>("!","TOKEN_!"),
    pair<string,string>("^","TOKEN_^"), //
    pair<string,string>("*","TOKEN_*"),
    pair<string,string>("/","TOKEN_/"),
    pair<string,string>("%","TOKEN_%"),
    pair<string,string>("+","TOKEN_+"),
    pair<string,string>("-","TOKEN_-"),
    pair<string,string>("<","TOKEN_<"),
    pair<string,string>("<=","TOKEN_<="),
    pair<string,string>(">","TOKEN_>"),
    pair<string,string>(">=","TOKEN_>="),
    pair<string,string>("==","TOKEN_=="),
    pair<string,string>("\\n","TOKEN_newline"), //
    pair<string,string>("!=","TOKEN_!="),
    pair<string,string>("=","TOKEN_="),
    pair<string,string>("&&","TOKEN_&&"),
    pair<string,string>("||","TOKEN_||"),
    pair<string,string>(";","TOKEN_;"),
    pair<string,string>("\'","TOKEN_Comilla"),
    pair<string,string>("\"","TOKEN_Comilla_doble"),
    pair<string,string>(",","TOKEN_,"),
    pair<string,string>("//","TOKEN_COMLINE"),
    pair<string,string>("{","TOKEN_{"),
    pair<string,string>("}","TOKEN_}"),
    pair<string,string>(":","TOKEN_:"),
    pair<string,string>("/*","TOKEN_COMBLOCKS"), //COMMENT BLOCK START
    pair<string,string>("*/","TOKEN_COMBLOCKE"), //   ''    ''   END

};

bool Find_char(char a) {
    if (isalpha(a)|| int(a) == 95) { return true; }
    else return false;  //revisa que sea char permitido
}

bool Find_int(char a) {
    if (isdigit(a)) { return true; }
    else return false;
} // revisa que sea int permitido

bool Find_symbol(char a) {  //lo mismo pero para simbolos ya que tipo para errores de id como hol@c deberia salir fallo pero cortaba de frente en el hol, asi q con esto
    //quiero hacer que los lea normal pero cuando es uno de estos simbolos pues que sepa q es un simbolo que si es perteneciente al lenguaje por lo que puede tener otro token
    string symbols = "[]+-*/%=!&|<>;:{}() \'\",\n\r\0\\";
    if (a == '\0') return true;
    return symbols.find(a) != string::npos;
}


void read_file(const string& filename, char*& content, int& size) {
    ifstream file(filename, ios::binary | ios::ate);
    if (file.is_open()) {
        size = file.tellg();
        file.seekg(0, ios::beg);  //regresa al inicio

        content = new char[size];
        file.read(content, size);
        file.close();
    }
    else {
        std::cerr << "No se puede abrir el archivo :c" << std::endl;
        content = nullptr;
        size = 0;
    }
}


char get_char() {
    if (indexx < sizee) {
        return text_Arr[indexx++]; //agarra el actual y mueve el puntero
    }
    else { return '\0'; }
}

char peek_char() {
    if (indexx + 1 < sizee) {  // revisa el siguiente sin mover puntero
        int tmp = indexx + 1;
        return text_Arr[tmp];
    }
    return '\0';

}

char peek_actual_char() {
    if (indexx < sizee) {
        return text_Arr[indexx];   // revisa el actual sin mover puntero
    }
    return '\0';
}


void scanner(char* buffer) {
    string word;
    char letter;
    while (indexx < sizee) {

        letter = peek_actual_char();
        word = "";
        if (letter == '\n' ) {
            get_char(); //salta espacios en blanco y eso
            line_global++;
            col_global = 0;
            continue;
        }
        if (letter == ' ' || letter == '\r') {
            col_global++; get_char();
            continue;
        }
        else if (Find_char(letter) == true) {
            bool valid_identifier = true;
            while (Find_char(peek_char()) == true || Find_int(peek_char()) == true || Find_symbol(peek_char()) == false) {
                word += get_char();
                col_global++;// si encuentra algo que no cuadra, tipo un simbolo que no pertenece al alfabeto
                if (!Find_char(peek_actual_char()) && !Find_int(peek_actual_char())) {
                    valid_identifier = false;
                }
            }
            word += get_char();
            col_global++;
            if (tokens_list.find(word) != tokens_list.end()) {
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<< tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else if (valid_identifier) { //si es valido se guarda como id
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<"TOKEN_ID"<< endl;
                tokens.push_back(token("TOKEN_ID"));
            }
            else { 
                cout << "Error: Identificador invalido '" << word << " linea :" << line_global << " columna: " << col_global << endl;
            }
        }

        else if (Find_int(letter) == true) {
            bool valid_identifier = true;
            while (Find_int(peek_char()) == true || Find_symbol(peek_char()) == false) {
                word += get_char();
                col_global++;
                // Si encontramos algo que no es un número ni un símbolo permitido, es error
                if (!Find_int(peek_actual_char()) && !Find_symbol(peek_actual_char())) {
                    valid_identifier = false;
                }
            }
            word += get_char();
            col_global++;// Añadir el último carácter leído
            if (valid_identifier) {
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<"TOKEN_Num"<< endl;
                tokens.push_back(token("TOKEN_Num"));
            }
            else {
                cout << "Error: Numero invalido '" << word << "linea :" << line_global << " columna: " << col_global << endl;
            }
        }

        else if (letter == ' ') {
            get_char();
        }
        else if (letter == '/') {
            if (peek_char() == '*') {
                word += get_char();
                word += get_char();
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
                
                int nested_com = 1;
                while (nested_com > 0) {
                    get_char();
                    col_global++;
                    if (peek_actual_char() == '\n') { line_global++; col_global = 0; }
                    else if (peek_char() == '*' && peek_actual_char() == '/') { // inicio de comentarios anidados 
                        get_char();
                        get_char();
                        col_global++;
                        nested_com++;
                    }
                    else if (peek_char() == '/' && peek_actual_char() == '*') { // fin de comentarios anidados
                        get_char(); 
                        get_char();
                        col_global++;
                        nested_com--;
                    }
                }
                word = "*/";
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else if (peek_char() == '/') {
                word += get_char();
                col_global++;
                word += get_char();
                col_global++;//porque son 2 simbolos
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
                while (peek_actual_char() != '\n') {
                    get_char();
                    col_global++;//salta todo hasta q termine la linea
                }
            }
            else { word += get_char(); col_global++; 
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
            tokens.push_back(token(tokens_list[word]));}
        }
        else if (letter == '\\') {
            if (peek_char() == 'n') {
                word += get_char();
                col_global++;
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else {
                cout << "falta ponerle algo mas al \\ no querras decir \\n?" << " linea :" << line_global << " columna: " << col_global << endl;
            }
        }

        else if (letter == '+') {
            if (peek_char() == '+') {
                word += get_char();
                col_global++;
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else {
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
        }

        else if (letter == '-') {
            if (Find_int(peek_char()) == true) {
                word += get_char();
                col_global++;
                bool valid_identifier = true;
                while (Find_int(peek_char()) == true || !Find_symbol(peek_char())) {
                    word += get_char();
                    col_global++;
                    if (!Find_int(peek_actual_char()) && !Find_symbol(peek_actual_char())) {
                        valid_identifier = false;
                    }
                }
                word += get_char();
                col_global++;
                if (valid_identifier) {
                    cout << word << " TOKEN_Num" << endl;
                }
                else {
                    cout << "Error: Número invalido '" << word << " linea :" << line_global << " columna: " << col_global << endl;
                }
            }
            else {
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
        }
        else if (letter == '[') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }

        else if (letter == ']') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }
        else if (letter == '(') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }

        else if (letter == ')') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
        else if (letter == '{') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }
        else if (letter == '}') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }
        else if (letter == '^') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }
        else if (letter == '*') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }
        else if (letter == '%') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }
        else if (letter == '<') {
            
            if (peek_char() == '=') {
                word += get_char();
                word += get_char();
                col_global++;
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else
            {
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
        }
        else if (letter == '>') {
            
            if (peek_char() == '=') {
                word += get_char();
                word += get_char();
                col_global++;
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else
            {
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
        }
        else if (letter == '!') {
            
            
            if (peek_actual_char() == '=') {
                word += get_char();
                word += get_char();
                col_global++;
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else
            {
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
        }
        else if (letter == '=') {
            
            if (peek_char() == '=') {
                word += get_char();
                word += get_char();
                col_global++;
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else
            {
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
        }
        else if (letter == '&') {

            if (peek_char() == '&') {
                word += get_char();
                word += get_char();
                col_global++;
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else
            {
                word += get_char();
                col_global++;
                cout << "No querras decir && en lugar de " << word << "?" << " linea :" << line_global << " columna: " << col_global << endl;
            }
        }

        else if (letter == '|') {
            if (peek_char() == '|') {
                word += get_char();
                word += get_char();
                col_global++;
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else
            {
                word += get_char();
                col_global++;
                cout << "No querras decir || en lugar de" << word << "?" << " linea :" << line_global << " columna: " << col_global << endl;
            }
        }

        else if (letter == ';') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }

        else if (letter == ':') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }
        else if (letter == '\'') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }
        else if (letter == '\"') {
            word += get_char();  
            col_global++;
            word = "";
            while (peek_actual_char() != '\"' && peek_actual_char() != '\n' && peek_actual_char() != '\0') {
                word += get_char();
                col_global++;
            }
            if (peek_actual_char() == '\"') {
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list["\""]<< endl;
                tokens.push_back(token(tokens_list["\""]));
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<"TOKEN_Text_string"<< endl;
                tokens.push_back(token("TOKEN_Text_string"));
                
                word = "";
                word += get_char();
                col_global++;
                cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
            }
            else {
                cout << "Error: String sin cerrar '" << word << "'" << " linea :" << line_global << " columna: " << col_global << endl;
            }
        }
        else if (letter == ',') {
            word += get_char();
            col_global++;
            cout <<"Linea "<<line_global<<" Col "<<col_global<<"\t"<<tokens_list[word]<< endl;
                tokens.push_back(token(tokens_list[word]));
        }

    }
}

#endif
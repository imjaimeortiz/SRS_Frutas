#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>

using namespace std;

typedef struct Atributo {
    string nombre;  // Nombre del atributo
    string tipo;    // Tipo del atributo : Nom, NU
    string op;      // Si tipo == Nom, op es "=". Si tipo == NU, op puede ser "=, >=, <=, <, >"
    string valores; // Valores que puede tomar (En caso de que sea Nom)
} Atributo;

typedef struct Hecho {
    string nombre;  // Nombre del hecho
    string op;      // El operador siempre será "=", pero lo almacenamos para no tener problemas cuando vayamos leyendo
    string valor;   // Valor del hecho
} Hecho;

typedef struct Regla {
    string nombre;              // Nombre de la regla (identificador de la forma RX:)
    list<Atributo> condiciones; // Lista de atributos condición o antecedentes
    Hecho accion;               // Hecho acción o consecuente
    int prioridad;              // Prioridad de la regla
} Regla;

typedef struct Configuracion {
    list<Atributo> lista_atributos; // Lista de atributos con los que vamos a trabajar
    string meta;                    // Meta u objetivo
    vector<int> lista_prioridades;  // Lista con los valores de las prioridades. Cada valor está asociado a una regla de forma 1-1 en orden de aparición
} Configuracion;

typedef struct Base_Conocimientos {
    list<Regla> lista_reglas;   // Lista de reglas extraídas de la base del conocimiento
    string dominio;             // Dominio en el que estamos trabajando
} Base_Conocimientos;

string pasos_razonamiento;  // Pasos de las decisiones que vamos tomando en el encaminamiento para la Salida1
string fichero_2;           // Contenido de la Salida2

/**
 * Redefinición de la operación de comparación para poder comparar dos reglas. Esto se usa internamente cuando eliminamos una regla de la lista
**/
bool operator==(const Regla &r, const Regla &r2) {
    return r.nombre == r2.nombre; 
}
/**
 * Tipo del atributo que estamos almacenando. Se buscará en la lista de atributos de la configuración. El return "" nunca se ejecutará ya que sabemos que todos los atributos tratados se encuentran en la lista, es simplemente para evitar inconsistencias
**/
string comprobar_tipo (Configuracion c, string atributo) {
    for (Atributo a : c.lista_atributos)
        if (a.nombre == atributo)
            return a.tipo;
    return "";
}

/**
 * Lectura del fichero de la base de conocimiento
**/
Base_Conocimientos leer_base_conocimientos (Configuracion c, string fichero) {
    ifstream entrada (fichero);
    Base_Conocimientos base_conocimientos;
    int n_reglas;   // Número de reglas que nos encontramos
    string si_condicion;    // Variable en la que guardamos el "Si"
    string entonces_accion; // Variable en la que guardamos el "Entonces"
    getline(entrada, base_conocimientos.dominio);  // Almacenamos el dominio de la aplicación
    fichero_2 += "Dominio : " + base_conocimientos.dominio + "\n\n";
    entrada >> n_reglas;
    list<Atributo> lista_atributos;
    for (int i = 0; i < n_reglas; i++) {
        Regla regla;
        entrada >> regla.nombre;    // Almacenamos el nombre de la regla
        entrada >> si_condicion;
        if (si_condicion == "Si"){ // Si hemos leído "Si" hasta que leamos "Entonces" vamos construyendo atributos y almacenándolos en la lista de condiciones de la regla
            do {
                Atributo a;
                entrada >> a.nombre;
                a.tipo = comprobar_tipo(c, a.nombre);
                entrada >> a.op;
                entrada >> a.valores;
                lista_atributos.push_back(a);
                entrada >> entonces_accion;
            } while (entonces_accion != "Entonces");
            Hecho h;    // Tras el "Entonces" encontramos un hecho en forma de consecuente.
            entrada >> h.nombre;
            entrada >> h.op;
            entrada >> h.valor;
            // Completamos los campos de la regla : condiciones, acción y prioridad
            regla.condiciones = lista_atributos;
            regla.accion = h;
            regla.prioridad = c.lista_prioridades.at(i);
            // Añadimos la regla a la lista de reglas y reseteamos la lista de condiciones para la siguiente regla
            base_conocimientos.lista_reglas.push_back(regla);
            lista_atributos.clear();
        } else {
            printf("El formato de la regla no es correcto\n");
            exit(EXIT_FAILURE);
        }
    }
    entrada.close();
    return base_conocimientos;
}

/**
 * Lectura del fichero de configuración
**/
Configuracion leer_configuracion (string fichero) {
    ifstream entrada (fichero);
    string palabra_reservada;   // Esta variable tomará los valores "ATRIBUTO", "OBJETIVO" y "PRIORIDADES-REGLAS"
    int n_atributos, n_reglas, prioridad;   // Variables para almacenar el número de atributos y de reglas con los que nos encontramos y la prioridad leída en cada iteración
    Configuracion c;    // Estructura configuración que contendrá la lista de atributos, el objetivo y la lista de prioridades
    entrada >> palabra_reservada;
    if (palabra_reservada == "ATRIBUTOS") { // Si leemos "ATRIBUTOS" sabemos que vamos a tener que almacenar n_atributos, por lo que hacemos un bucle para cada uno de ellos almacenando su estructura e introduciéndolos a la lista
        entrada >> n_atributos;
        for (int i = 0; i < n_atributos; i++) {
            Atributo a;
            entrada >> a.nombre;
            entrada >> a.tipo;
            if (a.tipo =="Nom") {  // Si el atributo es nominal, encontraremos a continuación la lista de posibles valores, si es numérico no encontramos nada
                entrada >> a.valores;
            } 
            c.lista_atributos.push_back(a);
        }
        entrada >> palabra_reservada;
    } 
    if (palabra_reservada == "OBJETIVO") {   // Si leemos "OBJETIVO" sabemos que a continuación tendremos la meta de nuestro problema
        entrada >> c.meta;
        entrada >> palabra_reservada;
    } 
    if (palabra_reservada == "PRIORIDADES-REGLAS") {    // Si leemos "PRIORIDADES-REGLAS" sabemos que vamos a tener un entero con el número de reglas y los n_reglas valores de prioridad
        entrada >> n_reglas;
        for (int i = 0; i < n_reglas; i++) {
            entrada >> prioridad;
            c.lista_prioridades.push_back(prioridad);
        }
    }
    entrada.close();
    return c;
}

/**
 * Lectura del fichero de la base de hechos
**/
list<Hecho> leer_base_hechos(string fichero) {
    ifstream entrada (fichero);
    int n_hechos;   // Número de hechos con los que nos encontramos
    list<Hecho> base_hechos;
    entrada >> n_hechos;
    pasos_razonamiento += "Hechos iniciales : ";
    for (int i = 0; i < n_hechos; i++) {    // Para cada hecho, construimos una estructura con sus campos que encontraremos seguidos y lo almacenamos en la base de hechos
        Hecho h;
        entrada >> h.nombre;
        entrada >> h.op;
        entrada >> h.valor;
        base_hechos.push_back(h);
        pasos_razonamiento += "[" + h.nombre + h.op + h.valor + "]";
    }
    entrada.close();        
    pasos_razonamiento += "\n\n";
    return base_hechos;
}

list<Regla> equiparar(Base_Conocimientos bc, list<Hecho> base_hechos) {
    list<Regla> conjunto_conflicto;
    for (Regla r : bc.lista_reglas) {  // Recorremos las reglas de la base del conocimiento
        list<Atributo> condiciones_r = r.condiciones;   
        bool coincidentes;
        bool posible_conflicto = true;      // Flag que se activará cuando falle un atributo. Sin él podría pasar que sólo cumpliendose el último se diera por válido.
        for (Atributo a : condiciones_r) {  // Para cada regla recorremos sus antecedentes
            coincidentes = false;
            if (posible_conflicto) {
                for (Hecho h : base_hechos) {   // Recorremos la base de hechos para comprobar la correspondencia atributo-hecho
                    if (a.nombre == h.nombre) {
                        if (a.tipo == "Nom") {  // tipo = Nom
                            if (a.valores == h.valor)
                                coincidentes = true;
                            else
                                posible_conflicto = false;                                                     
                        } else {    // tipo = NU
                            int valor_h = atoi(h.valor.c_str());
                            int valor_a = atoi(a.valores.c_str());
                            if (a.op == ">=" && valor_h >= valor_a) {
                                coincidentes = true;
                            }
                            else if (a.op == ">" && valor_h > valor_a){
                                coincidentes = true;
                            }
                            else if (a.op == "=" && valor_h == valor_a){
                                coincidentes = true;
                            }
                            else if (a.op == "<" && valor_h < valor_a){
                                coincidentes = true;
                            }
                            else if (a.op == "<=" && valor_h <= valor_a){
                                coincidentes = true;
                            }
                            else
                                posible_conflicto = false;                            
                        }
                        // tipo
                    }
                    // nombre
                }
                if (!coincidentes)
                    posible_conflicto = false;
                // cada hecho
            }
        }
        // Tras haber comprobado todas las condiciones de una regla : 
        if (coincidentes) {    // Cumple las condiciones para añadirlas al conjunto conflicto, todos los antecedentes se encuentran en la base de hechos
            conjunto_conflicto.push_back(r);
            pasos_razonamiento += "\t\tInsertamos [" + r.nombre + "] en el conjunto conflicto\n";
        }
     }        
    return conjunto_conflicto;
}

/**
 * Resolver el conflicto
**/
Regla resolver (list<Regla> conjunto_conflicto) {
    Regla regla;    // Definimos una regla base con un valor mínimo para proceder a comparar
    regla.nombre = "";
    regla.prioridad = -1;
    pasos_razonamiento += "\tConjunto conflicto a resolver : { ";
    for (Regla r :conjunto_conflicto)
        pasos_razonamiento += "[" +  r.nombre + "]";
    pasos_razonamiento += " }\n";
    for (Regla r : conjunto_conflicto) {
        if (r.prioridad > regla.prioridad) { // Si la regla que estamos comparando no es ella misma y su prioridad es mayor que la de la regla encontrada por el momento, actualizamos al regla a devolver
            regla = r;
        }
    }
    pasos_razonamiento += "\tResolvemos con " + regla.nombre + " (prioridad " + to_string(regla.prioridad) + ") el conjunto conflicto : { ";
    for (Regla r : conjunto_conflicto)
        pasos_razonamiento += "[" + r.nombre + "]";
    pasos_razonamiento += " }\n";    
    return regla;   // Devolvemos la regla con mayor prioridad del conjunto conflicto
}

/**
 * Devolver el hecho disparado por la regla
**/
Hecho aplicar(Regla r) {
    return r.accion;
}

/**
 * Actualización de la base de hechos con el hecho generado por la regla
**/
list<Hecho> actualizar_base_hechos (Hecho h, list<Hecho> base_hechos) {
    base_hechos.push_back(h);
    return base_hechos;
}

/**
 * Comprobamos si el objetivo que queríamos alcanzar se corresponde con algún hecho de la base de hechos
**/
bool contenida(string meta, list<Hecho> base_hechos) {
    for (Hecho h : base_hechos) {
        if (h.nombre == meta) {
            fichero_2 += "Objetivo : " + h.nombre + h.op + h.valor + "\n";
            pasos_razonamiento += "Objetivo alcanzado : " + h.nombre + h.op + h.valor + "\n";
            return true;
        }
    }
    return false;
}

/**
 * Algoritmo de encaminamiento hacia delante
**/
void encadenamiento_hacia_delante(Configuracion c, Base_Conocimientos bc, list<Hecho> base_hechos) {
    list<Regla> reglas_aplicadas;
    list<Regla> conjunto_conflicto;
    bool valida = false;
    int i = 1;
    do {
        pasos_razonamiento += "Iteracion : " + to_string(i++) + ": \n";   
        pasos_razonamiento += "\tReglas del conjunto conflicto inicialmente : { ";
        for (Regla r : conjunto_conflicto)
            pasos_razonamiento += "[" + r.nombre + "]";
        pasos_razonamiento += " }\n";
        conjunto_conflicto = equiparar(bc, base_hechos); // ConjuntoConflicto=ConjuntoConflicto+Equiparar(antecedente(BC),BH) (Los antecedentes se acceden desde cada una de las reglas de lista_reglas y la base de hechos desde base_hechos)
                for (Regla r : reglas_aplicadas) { // ConjuntoConflicto=ConjuntoConflicto-ReglasAplicadas;
            for (Regla r2 : conjunto_conflicto) {
                if (r.nombre == r2.nombre) {
                    conjunto_conflicto.remove(r);
                    pasos_razonamiento += "\t\tEliminada la regla " + r.nombre + " del conjunto conflicto por haber sido aplicada\n";
                }
            }
        }
        if (!conjunto_conflicto.empty()) {  // si NoVacio(ConjuntoConflicto) entonces
            Regla r = resolver(conjunto_conflicto); // R=Resolver(ConjuntoConflicto);
            Hecho nuevo_hecho = aplicar(r); // NuevosHechos=Aplicar(R,BH);
            reglas_aplicadas.push_back(r);  // ReglasAplicadas=ReglasAplicadas+{R};
            pasos_razonamiento += "\n\tObtenido nuevo hecho " + nuevo_hecho.nombre + nuevo_hecho.op + nuevo_hecho.valor + "\n";
            base_hechos = actualizar_base_hechos(nuevo_hecho, base_hechos);    // Actualizar(BH,NuevosHechos);
            pasos_razonamiento += "\n\nBase de hechos actualizada : ";
            for (Hecho h : base_hechos) {
                pasos_razonamiento += "[" + h.nombre + h.op + h.valor + "]";
            }
            pasos_razonamiento += "\n\n";
        }
        valida = (contenida(c.meta, base_hechos));
    } while (!valida && !conjunto_conflicto.empty());   // hasta Contenida(Meta,BH) o Vacio(ConjuntoConflicto); (Repetir mientras no se haya encontrado el objetivo y siga habiendo reglas en el conjunto_conflicto)
    
    if (valida) {   // si Contenida(Meta,BH) entonces devolver ‘’exito’’
        fichero_2 += "\nRazonamiento aplicado : ";
        for (Regla r : reglas_aplicadas) 
            fichero_2 += "\tRegla " "[" + r.nombre + "]" + " con prioridad " + to_string(r.prioridad);
    }
    else 
        pasos_razonamiento += "\nNo se ha podido alcanzar el objetivo " + c.meta;
}

int main (int argc, char **argv) {

    Configuracion c = leer_configuracion(argv[1]);
    Base_Conocimientos bc = leer_base_conocimientos(c, argv[3]);
    list<Hecho> bh = leer_base_hechos(argv[2]);

    string basename(argv[2]);
    basename = basename.substr(basename.size() - 6, basename.size());
    string nombre_salida_1 = "Salida1-" + basename;
    string nombre_salida_2 = "Salida2-" + basename;

    
    ofstream salida1 (nombre_salida_1);
    encadenamiento_hacia_delante(c, bc, bh); 
    salida1 << pasos_razonamiento;
    salida1.close();

    ofstream salida2 (nombre_salida_2);
    salida2 << fichero_2;
    salida2.close();
   
}
#include <vector>
#include <bitset>
#include <array>

export module dat_test;


struct DAG {
public:
    DAG(int n)
    {
        adj_matrix = std::vector<std::vector<int>>(n, std::vector<int>(n, 0));
    }



public:

    bool createsCycleUtil() {

    }


    void add_edge(int u, int v) {

    }


private:
    std::vector<std::vector<int>> adj_matrix;




};

// faire un fileRegistry ou un moduleRegistry



// en gros, faire un std::vector<std::bitset<N>> ou chaques bits du bitset sont un coolean
// ensuite, on fait un système qui donne un id précis (probablement atomic counter) a chaques
// files, probablement faire un autre vecteur (ou array étant donné qu'on connait N) a coté qui
// stocke les vrais données, un peu comme 

// d'ailleurs, on pourra flatten le graph

static constexpr std::size_t matrice_size = 10;


struct DAG2 {
public:
    // ici, on prends en compte qu'Ils sont deja int mais il nous faut un système 
    // extérieur qui donne un id a chaque fichier.

    // peut etre uniquement stocker un id dans le ficheir et passer a un modèle totalement DoD.
    // mais ça implique plusieurs couts, il faudrait étudier la chose.

    void add_edge(std::size_t u, std::size_t v) {
        flattened.set(u * matrice_size + v); // sets the bit at u, v to 1
        flattened.set(v * matrice_size + u); // sets the bit at v, u to 1
    }

    void remove_edge(std::size_t u, std::size_t v) {
        flattened.reset(u * matrice_size + v);
        flattened.reset(v * matrice_size + u);
    }

    // faire une function qui regarde si inclusion circulaire
    // ne gros, on fait genre dfs, on prends chaque adjacent et 
    // on explore jusqu'a touver un truc qui reviens deux fois


    bool dfs() {

        // en gros on a un indegree qui est un std::bitset<N>
        // et nous avons un visited qui est aussi un std::bitset<N>

        // niveau optimisation, on les fait SHARED, on ne les passe pas
        // récursivement dans la fonciton, on les utilise jusqu'a la fin 
        // et on reset les deux bitset a la fin.


    }


    std::vector<std::size_t> neighbors(std::size_t u) const {




    }

    std::size_t degree(std::size_t u) const {
        flattened.count(); // counte sur tout le bitset

        // il faut compter sur une plage précise. 
        // soit découper en plus petits blocs avec un peu de magie 
        // ou refaire le modèle de stockage.



    }

private:
    std::bitset<matrice_size * matrice_size> flattened;

    // PROBABLEMENT MOYEN D'OPTIMISER ENCORE PLUS AVEC SIMD, 
    // IL PARAIT QU'IL FAUT UTILISER DES uint64_t POUR LE FAIRE.

    // std::vector<std::uint64_t> ou flatten directement

};


// pour le système, il faut faire du DoD pour le fichier, genre mettre un ID
// dans le fichier, et utiliser des systèmes pour obtenir les données de dehors

// il y a l'option de mettre les données dans le struct avec le id aussi mais
// il n'y a aucun moyen d'obtenir les données du noeud par rapport au id si celui-ci
// est enfermé à l'intérieur d'un objet. ainsi, il faut externaliser les données.

/******************************************************************************************/

struct atomic_registry {

    // permet d'obtenir les données du file a partir de son id

    // peut être que C'EST JUSTEMENT LUI LE FileDataRegistry.

    // UTILISER LE SYNCHRONIZED_MONOTONIC_BUFFER_RESSOURCE. (atomic monotonic buf)

};

struct FileData_changer_le_nom {
    std::string content;
    std::string name;
};

struct File_changer_le_nom {
    std::size_t id;
};


// il s'agirait de faire un flattened graph mais pour l'instant, on garde ça simple.
#include <filesystem>
export module file_discovery_system;

using mock_t = int;


// ici, la tache est simplement de extraire les PATH des fichiers et en leur donnant un id
// cette section ne fait pas partie du pipeline multi threaded, il est mono-threaded


// premier module est UNIQUEMENT pour extract les path
// le deuxième module ensuite sert a attribuer un id unique à chacuns des paths

// faudrait aussi remplir le indexer de noms et path de fichiers genre un FileIndex ou FileIndexer



struct SourceEntry {
    std::filesystem::path path;
    std::size_t id;
};


#if 0
struct path_extraction_pass {
    using RequiredArgument = mock_t;

    using RequiredContext = mock_t;
    using RequiredDependency = mock_t;

    static std::vector<std::filesystem::path>& execute(RequiredArgument& arg, RequiredContext& ctx, RequiredDependency& dep) {
        //dep.parse(ctx);

        return GLOBAL_CHAR; // move peut etre

        // lui il return uniquement un std::vector<path>, et non les FileEntry directement
    }

};
#endif

// source_id_attribution_pass

// source_indexing_pass

/**************************************************************/

// PATH_EXTRACTION_PASS:
//      Entrée -> une racine sous forme de path
//      Sortie -> un vecteur de paths

// SOURCE_ID_ATTRIBUTION_PASS:
//      Entrée -> un vecteur de paths
//      Sortie -> un vecteur de SourceEntry

// SOURCE_INDEXING_PASS:
//      Entrée -> un vecteur de SourceEntry
//      Sortie -> un vecteur de SourceEntry (aucune modification)

/**************************************************************/

struct PathExtractor {

};

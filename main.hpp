#include <iostream>

//import std;
//import std.compat;

#include <tuple>

//import std;

import thread;

// un unit de compilation contient les ressources locales ainsi que certaines 
// ressources shared. il s'agirait de faire un context local et un context shared.

// le pipeline devrait uniquement être local
// donc le pipeline est littéralement unit de compilation qui effectue les passes
// les steps du pipeline sont les passes. 

// base pour le nouveau design de l'ancienne structure du compilateur

using GLOBAL_MOCK_TYPE = unsigned char;

namespace llvm { using context = GLOBAL_MOCK_TYPE; }

using llvmBackend = GLOBAL_MOCK_TYPE;
using AST = GLOBAL_MOCK_TYPE;

using data_t = unsigned char;

/*****************************************************************/

enum ContextBits : std::uint32_t {
    Lexing = 1u << 0,
    Parsing = 1u << 1,
    Codegen = 1u << 2,
};

/*****************************************************************/

#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_constructive_interference_size;
    using std::hardware_destructive_interference_size;
#else
    // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
    constexpr std::size_t hardware_constructive_interference_size = 64;
    constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

/*****************************************************************/

template<bool Condition, typename Target, typename Predicate>
struct ConfigurationRule {
    static constexpr bool condition = Condition;

    using target = Target;
    using predicate = Predicate;
};

template<typename Target, typename Predicate>
using ENABLED = ConfigurationRule<true, Target, Predicate>;

template<typename Target, typename Predicate>
using DISABLED = ConfigurationRule<false, Target, Predicate>;

template<auto Condition, typename Target, typename Predicate>
using CONDITIONAL = ConfigurationRule<Condition, Target, Predicate>;

// RULE
// CLAUSE
// POLICY

/*****************************************************************/

// NOUVEAU NOM POUR LE monotonic_atomic_buffer

struct unsynchronized_monotonic_buffer_ressource {};
struct synchronized_monotonic_buffer_ressource {};

// OU BIEN unsychronized_monotonic_allocator

/*****************************************************************/

template<std::size_t N>
struct ShardedStringInterner {

    [[nodiscard]] const char* const intern(std::string_view sv) {
        std::hash<std::string_view> h; std::size_t index = h(sv) % N;

        return "mock_string";
    }


};

/*****************************************************************/

struct SharedContext {
    constexpr SharedContext() {}


    data_t shared_ressource;

    std::uint8_t trace;

    // dans le shared, on peux mettre par exemple:
    //      - un systeme de debugging
    //      - peut-être un allocator "fourre-tout"
    //      - SURTOUT le StringInterner
    //      - la table de symbole


    ShardedStringInterner<10> stringInterner;
};



// peut etre faire une classe abstraite avec le SharedContext& et constructeur approprié


struct alignas(hardware_constructive_interference_size) LexingContext {
    explicit LexingContext(SharedContext& s)
        : shared(s) {} // temporaire attendant abstract class

public:
    SharedContext& shared;

    data_t CSTAllocator; // changer le nom mais ce n'est pas exactement CST

    //data_t Allocator;
};

struct alignas(hardware_constructive_interference_size) ParsingContext {
    explicit ParsingContext(SharedContext& s)
        : shared(s) {}

public:
    SharedContext& shared;

    data_t ast;
    data_t ChunkAllocator;

    // mettre les allocators et tous les systèmes requis au contexte
};

struct alignas(hardware_constructive_interference_size) CodegenContext {
    explicit CodegenContext(SharedContext& s)
        : shared(s) {}

public:
    SharedContext& shared;

    // il ne faut jamais stocker l'ast ou un ARG venant de la passe
    // précédente dans les contextes des passes suivantes. pourquoi?

    // c'est simplement une question de ownership, le mémoire de l'ast
    // est allocated dans le arena allocator du contexte précédent.
     
    // nous devons consommer l'ast au lieu de le stocker, c'est le seul
    // compromis possible si je souhaite respecter le modèle mémoire actuel.
};


template<typename... Models>
struct ContextProvider {

    SharedContext shared;
    std::tuple<Models...> contexts;

    constexpr ContextProvider()
        : shared{}
        , contexts(Models{ shared }...)
    {}

public:
    template<typename T>
    constexpr T& get() noexcept {
        return std::get<T>(contexts);
    }
};

// faire aussi un ModuleProvider pour l'autre (faire un wrapper au lieu de std::tuple directement)

/*****************************************************************/

template<typename... Models>
struct ModuleProvider {
    std::tuple<Models...> modules{};

    template<typename T>
    constexpr T& get() noexcept {
        return std::get<T>(modules);
    }
};

/*****************************************************************/


struct console_logger {

    static auto log(std::string_view data) {
        std::cout << "console_logger::log -> " << data << "\n";
    }

    template<typename T>
    static auto log(T&& data) {
        std::cout << "console_logger::log -> " << data << "\n";
    }

};


// injecter les rules SUR LE TYPE, pas dans la méthode
struct mock_lexer_0 { // LexingEngine ou LexerEngine (lui)

    void lex(LexingContext& ctx) {
        std::cout << "calling mock_lexer_0::lex\n";

        ctx.shared.trace += 10;
    }

};

struct mock_parser_0 {

    void parse(ParsingContext& ctx) {
        std::cout << "calling mock_parser_0::parse\n";

        ctx.shared.trace += 10;

        //ctx.shared.str += std::string("<executing mock_parser_0> ");

    }

};


static unsigned char GLOBAL_CHAR = 'c';

struct ObjectFileSource {
    // name, module name et content
};

struct ObjectFileArtifact {
    std::string path; // probablement un const char* allocated dans le interning
};



// context pas type-safe, mettre concept à la place
template<typename MProvider, typename CProvider, typename... Components>
struct comp_unit_pipeline { // execution engine ou quelque chose avec engine genre compilation engine
    // compilation orchestrator??? ou compilation engine

    MProvider moduleProvider;
    CProvider contextProvider;

    explicit comp_unit_pipeline(MProvider& m, CProvider& c)
        : moduleProvider(m)
        , contextProvider(c)
    {}


    template<typename T>
    decltype(auto) run(T& arg) { // return par copie, pas par reference
        return this->template run_impl<Components...>(arg); // copie elision, RVO (pas garanti)
    }


    template<typename FirstStep, typename... Rest>
    decltype(auto) run_impl(typename FirstStep::RequiredArgument& arg) {
        
        auto& result = FirstStep::execute(arg,
            contextProvider.get<typename FirstStep::RequiredContext>(),
            moduleProvider.get<typename FirstStep::RequiredDependency>()
        );

        //static_assert()

        // genre faire un struct pipeline et faire genre un utilitaire qui check si
        // OUT du step1 est étal a IN du step2, et ainsi de suite.

        // ajouter un concept + un assert pour vérifier si le result est compatible
        // avec le prochain step. OU JUSTE STATIC ASSERT std::is_same_as

        if constexpr (sizeof...(Rest) > 0) {
            return this->template run_impl<Rest...>(result); // std::invoke
        }
        else {
            return result;
        }
    }

};

/*****************************************************************/


struct tokenizing_pass_0 {
#if 0
    struct traits {
        using RequiredArgument = file_info;

        using RequiredContext = ParsingContext;
        using RequiredDependency = mock_parser_0;
    };
#endif

    using RequiredArgument = ObjectFileSource;

    using RequiredContext = ParsingContext;
    using RequiredDependency = mock_parser_0;

    static data_t& execute(RequiredArgument& arg, RequiredContext& ctx, RequiredDependency& dep) {
        dep.parse(ctx);

        return GLOBAL_CHAR;
    }

};


struct lexing_pass_0 { // ici c'est plus concrete trre builder pass ou qqch du genre
    using RequiredArgument = data_t;

    using RequiredContext = LexingContext;
    using RequiredDependency = mock_lexer_0; // POSSIBLEMENT remplacer par un concept


    // return genre le ast ou quelque chose du genre, pas le context du tout
    static data_t& execute(RequiredArgument& arg, RequiredContext& ctx, RequiredDependency& dep) {
        dep.lex(ctx);

        return GLOBAL_CHAR;
    }

};


struct parsing_pass_0 { // ici c'est plus ast lowering pass
    using RequiredArgument = data_t; // IN
    //faire genre using pour IN et le OUT

    using RequiredContext = ParsingContext;
    using RequiredDependency = mock_parser_0;

    static data_t& execute(RequiredArgument& arg, RequiredContext& ctx, RequiredDependency& dep) {
        dep.parse(ctx);

        return GLOBAL_CHAR;
    }

};

struct ir_generation_pass_0 {
    using RequiredArgument = data_t;

    using RequiredContext = ParsingContext;
    using RequiredDependency = mock_parser_0;

    static ObjectFileArtifact& execute(RequiredArgument& arg, RequiredContext& ctx, RequiredDependency& dep) {
        static ObjectFileArtifact actifact; // temporaire, uniquement pour lifetime

        return actifact;
    }
};


struct context_logging_pass {
    using RequiredArgument = data_t;

    using RequiredContext = ParsingContext;
    using RequiredDependency = console_logger;

    static data_t& execute(RequiredArgument& arg, RequiredContext& ctx, RequiredDependency& dep) { // copie intentionnel pour mocking 
        dep.log(ctx.shared.trace);

        return GLOBAL_CHAR;
    }

};


/*****************************************************************/


struct BuildOrchestrator {
private:
    using ModuleProvider_ = ModuleProvider<mock_lexer_0, mock_parser_0, console_logger>;
    using ContextProvider_ = ContextProvider<LexingContext, ParsingContext, CodegenContext>;

    using CompilationUnit = comp_unit_pipeline<
        ModuleProvider_, ContextProvider_,

        tokenizing_pass_0, lexing_pass_0, parsing_pass_0, context_logging_pass, ir_generation_pass_0
    >;

private:
    ThreadPool pool;
    data_t systeme_de_linkage;

    ModuleProvider_ moduleProvider{};
    ContextProvider_ contextProvider{};

    CompilationUnit compilationEngine{ moduleProvider, contextProvider }; // un peu étrange comme nom
    // juste pour mock, en réalité, c'est plus complexe que ca

public:
    auto execute() {
        extraction();
        compilation();
        linkage();
    }

protected: // noms temporaires
    void extraction() {

    }

    void compilation(/*argument*/) {
        ObjectFileSource source;

        static_assert(
            std::is_same_v<
                std::decay_t<decltype(std::declval<CompilationUnit>().run(source))>,
                ObjectFileArtifact
            >,
            "Build failure: compilation pipeline did not produce ObjectFileArtifact."
        );

        compilationEngine.run(source);
    }

    void linkage() {

    }
};

/*****************************************************************/



template<typename... Ts>
struct Pipeline {};



inline static constexpr std::tuple<mock_lexer_0, mock_parser_0, console_logger>
    CompilationModules{ mock_lexer_0{}, mock_parser_0{}, console_logger{} };


// faire un type pipeline et utiliser using pour le mettre dans le truc

// aussi, faire un context shared et un local, les mettre les deux dans un package
// qui contient tous les autres modules, jsp a voir

// aussi, ne pas mettre ast et ir dans context, il faut les mettre comme une chaine
// dans les steps genre step1 return un ast que le step2 recoit et celui-ci retourne
// le ir, que le step3 recoit, ne jamais le stocker au fur et a mesure. 

// utiliser le systeme de composition de pipeline pour un futur système de commande
// permettant l'ajout d'arguments pour debug la compilation par exmeple (ajouter un step
// de debug en plus).

// faire un Application ou quelque chose par dessus qui s'occupe d'encapsuler tout ce que
// je fais déja dans le main. Peut etre que c'est lui qui permettra



// ORCHESTRATEUR INCLUDE : c'est lui qui fait le for each threads et qui crée un comp unit
//                         en gros il include les fichiers un par un dans la compilation

// CONFIGURATION FACADE ENVIRONNEMENT : c'est lui qui crée tous les 

// UNIT DE COMPILATION : il recoit tous les modules en paramètre de constructeur
//                       il crée une facede environnement avec certains params (à regarder)
//                       c'est lui qui effectue les pass, donc le pipeline.

// Notre modèle est différent, on a un pipeline et un contexte a coté. Plus simple et 
// bien plus extensible. C'est le design souhaité, rien de plus complexe n'est nécessaire.

/*****************************************************************************/

// Technique utilisée par le système de Unreal Engine 5
// pour le string interning, je dois retourner une structure spécifique au lieu de uniquement
// des const char*. En gros, au lieu de réserver uniquement la taille de n charactères, on alloc
// taille d'un entier (probablement un short 2 bytes) qu'on met AVANT la chaine de charactère, 
// ce qui donne [ index ][ charactères ], ensuite, faut un compteur qui fait index++ a chaque alloc
// (le mettre dans le StringInterner) et qui permet des optimisations comme utiliser des id dans un
// std::vector<bool> (est un bitset en arrière) au lieu de std::unordered_set pour savoir si par exemple
// une variable a déja été définit dans le scope, on regarde si le bool a l'index du string trouvé
// est à true. On fait un std::vector<bool> PAR scope.


/*****************************************************************************/

// pour les arguments commande runtime, il faut faire le système que j'avais testé
// précédemment ou on génère toutes les combinaisons du pipeline possible et on 
// ne fait que choisir en condition des arguments la prochaine combinaison correspondante.
 
/*****************************************************************************/

// LexicalAnalysisEngine
// SyntacticAnalysisEngine
// ASTConstructionEngine
// SemanticAnalysisEngine
// IRLoweringEngine

// OU BIEN (je préfère le premier)

// Lexer
// Parser
// ASTBuilder
// SemanticAnalyzer
// IRLowering

/*****************************************************************************/

// NOTE: utiliser le truc avec enabled ou ENABLED pour les configurations
//       en gros, on fait un type genre ConditionalConfigurationEntry et 
//       using Enabled = ...<T1, T2, true>, using Disabled = ...<T1, T2, false>,
//       using Conditional = ...<T1, T2, std::conditional<...>>. IMPORTANT!!!!!!

//       Pour remplacer le ParsingContext que j'avais mis partout dans l'ancien système.

/*****************************************************************************/

// Clarification du modèle:

// La compilation -> en partant du SourceFile vers le .o
// Le processus de build -> lecture et extraction + compilation + linkage

// Le LINKER LLD prends en argument les path de tous les fichiers .o générés.
//      son travail est basically de résoudre les unresolved declarations 
//      ainsi que de générer l'exécutable final à la fin.

// Il faut faire un abstraction que la dernière pass de génération de .o retournera
// et qui sera ObjectFileArtifact qui contient uniquement le path du .o (favorise extensibilité)  

// Enfin, le module qui gère la lecture, préparation des fichiers, assignations des tâches 
// aux threads depuis la thread pool et qui effectue le linkage final sera le module de Build.

/*****************************************************************************/

int main() {
    std::cout << "Hello World!\n";

    std::string input = "test0";

    //if (input == "test0") {
    //    typeA a;

    //    a.doSomething();
    //}
    //else if (input == "test1") {
    //    typeB b;

    //    b.doSomething();
    //}


    //Context ctx;


    //constexpr ModuleProvider<mock_lexer_0, mock_parser_0, console_logger> moduleProvider;    
    //
    //ContextProvider<
    //    LexingContext,
    //    ParsingContext,
    //    CodegenContext
    //> contextProvider{};



    /*******************************************************************/


    Pipeline<tokenizing_pass_0, lexing_pass_0, parsing_pass_0, context_logging_pass> p;

    // CompilationPipeline ou CompilationFlow ou CompilationOrder

    using CompilationPipeline = Pipeline<tokenizing_pass_0, lexing_pass_0, parsing_pass_0, context_logging_pass>;


    //using System = comp_unit_pipeline<
    //    CompilationModules, CompilationPipeline // EXEMPLE DE CE QUE CA DOIT DONNER
    //>; 
    
    // faut le passer en template type parameter


    /**********************************************************************/

    using ModuleProvider_ = ModuleProvider<mock_lexer_0, mock_parser_0, console_logger>;
    using ContextProvider_ = ContextProvider<LexingContext, ParsingContext, CodegenContext>;

    ModuleProvider_ moduleProvider{};
    ContextProvider_ contextProvider{};



    using CompilationUnit = comp_unit_pipeline<
        ModuleProvider_, ContextProvider_,

        tokenizing_pass_0, lexing_pass_0, parsing_pass_0, context_logging_pass, ir_generation_pass_0
    >;

    CompilationUnit unit{ moduleProvider, contextProvider }; // un peu étrange comme nom

    ObjectFileSource source;
    unit.run(source);

    //void end_ctx = unit.run();


    //std::cout << "context str result -> " << end_ctx.str << "\n"; // pour temp debug
}

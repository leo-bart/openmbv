// octave used M_PI which is not longer defined in newer compilers
#define _USE_MATH_DEFINES
#include <cmath>

// includes are somehow tricky with octave, see also Makefile.am

// include config.h
// we cannot add -I.. to the compiler options, see Makefile.am, hence we add -I.. and
// use this to include config.h
#include <../mbxmlutils/../config.h>

// normal includes
#include <functional>
#include <mbxmlutilshelper/dom.h>
#include <fmatvec/fmatvec.h>
#include <fmatvec/ast.h>
#include <xercesc/dom/DOMAttr.hpp>
#include "mbxmlutils/octeval.h"
#include "mbxmlutils/eval_static.h"
#include <boost/algorithm/string/trim.hpp>

#include <octave-config.h>
#undef OCTAVE_USE_DEPRECATED_FUNCTIONS
#include <octave/ovl.h>
#include <octave/interpreter.h>
#include <octave/octave.h>
#include <octave/symtab.h>
#include <octave/parse.h>
#include <octave/defaults.h>

using namespace std;
using namespace xercesc;
namespace bfs=boost::filesystem;
using namespace octave;

namespace {
  //TODO not working on Windows
  //TODO // NOTE: we can skip the use of utf8Facet (see below) and set the facet globally (for bfs::path and others) using:
  //TODO // std::locale::global(locale::generator().generate("UTF8"));
  //TODO // filesystem::path::imbue(std::locale());
  //TODO const bfs::path::codecvt_type *utf8Facet(&use_facet<bfs::path::codecvt_type>(locale::generator().generate("UTF8")));
  #define CODECVT

  // some platform dependent values
#ifdef _WIN32
  bfs::path LIBDIR="bin";
#else
  bfs::path LIBDIR="lib";
#endif

  bool deactivateBlock=getenv("MBXMLUTILS_DEACTIVATE_BLOCK")!=nullptr;

  // A class to block/unblock stderr or stdout. Block in called in the ctor, unblock in the dtor
  template<int T>
  class Block {
    public:
      Block(ostream &str_, streambuf *buf=nullptr) : str(str_) {
        if(deactivateBlock) return;
        if(disableCount==0)
          orgcxxx=str.rdbuf(buf);
        disableCount++;
      }
      ~Block() {
        if(deactivateBlock) return;
        disableCount--;
        if(disableCount==0)
          str.rdbuf(orgcxxx);
      }
    private:
      ostream &str;
      static streambuf *orgcxxx;
      static int disableCount;
  };
  template<int T> streambuf *Block<T>::orgcxxx;
  template<int T> int Block<T>::disableCount=0;
  #define BLOCK_STDOUT Block<1> MBXMLUTILS_EVAL_CONCAT(mbxmlutils_blockstdout_, __LINE__)(std::cout)
  #define BLOCK_STDERR Block<2> MBXMLUTILS_EVAL_CONCAT(mbxmlutils_blockstderr_, __LINE__)(std::cerr)
  #define REDIR_STDOUT(buf) Block<1> MBXMLUTILS_EVAL_CONCAT(mbxmlutils_redirstdout_, __LINE__)(std::cout, buf)
  #define REDIR_STDERR(buf) Block<2> MBXMLUTILS_EVAL_CONCAT(mbxmlutils_redirstderr_, __LINE__)(std::cerr, buf)
}

namespace MBXMLUtils {

MBXMLUTILS_EVAL_REGISTER(OctEval)

// Helper class to init/deinit octave on library load/unload (unload=program end)
class OctInit {
  public:
    OctInit();
    ~OctInit();
    string initialPath;
    unique_ptr<octave::interpreter> interpreter;
};

OctInit::OctInit() {
  try {
    // set the OCTAVE_HOME envvar and octave_prefix variable before initializing octave
    bfs::path octave_prefix;
    if(getenv("OCTAVE_HOME")) // OCTAVE_HOME set manually -> use this for octave_prefix
      octave_prefix=getenv("OCTAVE_HOME");
    else if(getenv("OCTAVE_HOME")==nullptr && bfs::exists(MBXMLUtils::Eval::installPath/"share"/"octave")) {
      // OCTAVE_HOME not set but octave is available in installation path of MBXMLUtils -> use installation path
      octave_prefix=MBXMLUtils::Eval::installPath;
      // the string for putenv must have program life time
      static string OCTAVE_HOME="OCTAVE_HOME="+MBXMLUtils::Eval::installPath.string(CODECVT);
      putenv((char*)OCTAVE_HOME.c_str());
    }
      // init interpreter
      interpreter.reset(new octave::interpreter());
    if(octave_prefix.empty())
      octave_prefix=config::octave_home();

    if(interpreter->execute()!=0)
      throw runtime_error("Cannot execute octave interpreter.");
  
    // set some global octave config
    octave_value_list warnArg;
    warnArg.append("error");
    warnArg.append("Octave:divide-by-zero");
    feval("warning", warnArg);
    if(error_state!=0) { error_state=0; throw runtime_error("Internal error: unable to disable warnings."); }

    // ... and add .../[bin|lib] to octave search path (their we push all oct files)
    string dir=(MBXMLUtils::Eval::installPath/LIBDIR).string(CODECVT);
    feval("addpath", octave_value_list(octave_value(dir)));
    if(error_state!=0) { error_state=0; throw runtime_error("Internal error: cannot add octave search path."); }

    // save initial octave search path
    initialPath=feval("path", octave_value_list(), 1)(0).string_value();
    if(error_state!=0) { error_state=0; throw runtime_error("Internal error: unable to get search path."); }

    // deregister __finish__, see OctEval::~OctEval
    octave_value_list atexitArg;
    atexitArg.append("__finish__");
    atexitArg.append(0);
    feval("atexit", atexitArg);
    if(error_state!=0) { error_state=0; throw runtime_error("Internal error: unable to get search path."); }
  }
  // print error and rethrow. (The exception may not be catched since this is called in pre-main)
  catch(const exception& ex) {
    fmatvec::Atom::msgStatic(fmatvec::Atom::Error)<<"Exception during octave initialization:"<<endl<<ex.what()<<endl;
    throw;
  }
  catch(...) {
    fmatvec::Atom::msgStatic(fmatvec::Atom::Error)<<"Unknown exception during octave initialization."<<endl;
    throw;
  }
}

OctInit::~OctInit() {
  try {
    // nothing needed for now (in octave 4.4)

    // __finish__.m which is run at exit is deregistered during octave initialzation via atext.
    // This is required to avoid running octave code after octave was alread removed.
    // (atexit is run on library unload at program exit)
  }
  // print error and rethrow. (The exception may not be catched since this is called in pre-main)
  catch(const exception& ex) {
    fmatvec::Atom::msgStatic(fmatvec::Atom::Error)<<"Exception during octave deinitialization:"<<endl<<ex.what()<<endl
      <<"Continuing but undefined behaviour may occur."<<endl;
  }
  catch(...) {
    fmatvec::Atom::msgStatic(fmatvec::Atom::Error)<<"Unknown exception during octave deinitialization."<<endl
      <<"Continuing but undefined behaviour may occur."<<endl;
  }
}

OctInit octInit; // init octave on library load and deinit on library unload = program end

inline shared_ptr<octave_value> C(const Eval::Value &value) {
  return static_pointer_cast<octave_value>(value);
}

inline Eval::Value C(const octave_value &value) {
  return make_shared<octave_value>(value);
}

string OctEval::cast_string(const Eval::Value &value) const {
  if(valueIsOfType(value, StringType))
    return C(value)->string_value();
  throw runtime_error("Cannot cast this value to string.");
}

double OctEval::cast_double(const Eval::Value &value) const {
  if(valueIsOfType(value, ScalarType))
    return C(value)->double_value();
  throw runtime_error("Cannot cast this value to double.");
}

vector<double> OctEval::cast_vector_double(const Eval::Value &value) const {
  if(valueIsOfType(value, ScalarType))
    return vector<double>(1, cast<double>(value));
  else if(valueIsOfType(value, VectorType)) {
    Matrix m=C(value)->matrix_value();
    vector<double> ret(m.rows());
    for(int i=0; i<m.rows(); ++i)
      ret[i]=m(i, 0);
    return ret;
  }
  throw runtime_error("Cannot cast this value to vector<double>.");
}

vector<vector<double> > OctEval::cast_vector_vector_double(const Eval::Value &value) const {
  if(valueIsOfType(value, ScalarType))
    return vector<vector<double> >(1, vector<double>(1, cast<double>(value)));
  if(valueIsOfType(value, VectorType)) {
    Matrix m=C(value)->matrix_value();
    vector<vector<double> > ret(m.rows(), vector<double>(1));
    for(int i=0; i<m.rows(); ++i)
      ret[i][0]=m(i,0);
    return ret;
  }
  else if(valueIsOfType(value, MatrixType)) {
    Matrix m=C(value)->matrix_value();
    vector<vector<double> > ret(m.rows(), vector<double>(m.cols()));
    for(int r=0; r<m.rows(); ++r)
      for(int c=0; c<m.cols(); ++c)
        ret[r][c]=m(r, c);
    return ret;
  }
  throw runtime_error("Cannot cast this value to vector<vector<double> >.");
}

Eval::Value OctEval::create_double(const double& v) const {
  return make_shared<octave_value>(v);
}

Eval::Value OctEval::create_vector_double(const vector<double>& v) const {
  Matrix m(v.size(), 1);
  for(int i=0; i<v.size(); ++i)
    m(i)=v[i];
  return make_shared<octave_value>(m);
}

Eval::Value OctEval::create_vector_vector_double(const vector<vector<double> >& v) const {
  Matrix m(v.size(), v[0].size());
  for(int r=0; r<v.size(); ++r)
    for(int c=0; c<v[r].size(); ++c)
      m(c*m.rows()+r)=v[r][c];
  return make_shared<octave_value>(m);
}

Eval::Value OctEval::create_string(const string& v) const {
  return make_shared<octave_value>(v);
}

Eval::Value OctEval::createFunctionDep(const vector<Value>& v) const {
  auto ret=createSwigByTypeName("VectorSym");
  auto vec=static_cast<fmatvec::Vector<fmatvec::Var, fmatvec::SymbolicExpression>*>(getSwigPtr(*C(ret)));
  vec->resize(v.size());
  for(int i=0; i<v.size(); ++i) {
    if(valueIsOfType(v[i], ScalarType))
      (*vec)(i)=C(v[i])->double_value();
    else {
      string type=getSwigType(*C(v[i]));
      if(type!="SymbolicExpression" && type!="IndependentVariable")
        throw runtime_error("Value is not scalar symbolic or independent variable.");
      (*vec)(i)=*static_cast<fmatvec::SymbolicExpression*>(getSwigPtr(*C(v[i])));
    }
  }
  return ret;
}

Eval::Value OctEval::createFunctionDep(const vector<vector<Value> >& v) const {
  auto ret=createSwigByTypeName("MatrixSym");
  auto mat=static_cast<fmatvec::Matrix<fmatvec::General, fmatvec::Var, fmatvec::Var, fmatvec::SymbolicExpression>*>(getSwigPtr(*C(ret)));
  mat->resize(v.size(), v[0].size());
  for(int r=0; r<v.size(); ++r)
    for(int c=0; c<v[r].size(); ++c)
      if(valueIsOfType(v[r][c], ScalarType))
        (*mat)(r,c)=C(v[r][c])->double_value();
      else {
        string type=getSwigType(*C(v[r][c]));
        if(type!="SymbolicExpression" && type!="IndependentVariable")
          throw runtime_error("Value is not scalar symbolic or independent variable.");
        (*mat)(r,c)=*static_cast<fmatvec::SymbolicExpression*>(getSwigPtr(*C(v[r][c])));
      }
  return ret;
}

Eval::Value OctEval::createFunction(const vector<Value> &indeps, const Value &dep) const {
  Cell c(indeps.size()+1,1);
  for(size_t i=0; i<indeps.size(); ++i)
    c(i)=*C(indeps[i]);
  c(indeps.size())=*C(dep);
  return C(c);
}

void* OctEval::getSwigPtr(const octave_value &v) {
  static octave_function *swig_this=octInit.interpreter->get_symbol_table().find_function("swig_this").function_value(); // get ones a pointer to swig_this for performance reasons
  // get the pointer: octave returns a 64bit integer which represents the pointer
  if(v.class_name()!="swig_ref")
    throw runtime_error("This value is not a reference to a SWIG wrapped object.");
  octave_value swigThis=fevalThrow(swig_this, v, 1, "Cannot get pointer to the SWIG wrapped object.")(0);
  return reinterpret_cast<void*>(swigThis.uint64_scalar_value().value());
}

Eval::Value OctEval::createSwigByTypeName(const string &name) {
  list<octave_value_list> idx;
  idx.emplace_back(name);
  idx.emplace_back();
  return C(fevalThrow(octInit.interpreter->get_symbol_table().find_function("new_"+name).function_value(), octave_value_list(), 1,
    "Failed to create "+name)(0));
}

string OctEval::getSwigType(const octave_value &v) {
  if(v.class_name()!="swig_ref")
    return "";
  // get the swig type (get ones a pointer to swig_type for performance reasons)
  static octave_function *swig_type=octInit.interpreter->get_symbol_table().find_function("swig_type").function_value();
  return fevalThrow(swig_type, v, 1, "Unable to get swig type.")(0).string_value();
}


string OctEval::serializeFunction(const Value &x) const {
  auto c=C(x)->cell_value();
  int nrIndeps=c.dims()(0)-1;
  stringstream str;
  str<<"f(";
  for(int i=0; i<nrIndeps; ++i) {
    string type=getSwigType(c(i));
    if(type=="IndependentVariable")
      str<<(i==0?"":",")<<*static_cast<fmatvec::IndependentVariable*>(getSwigPtr(c(i)));
    else if(type=="VectorIndep")
      str<<(i==0?"":",")<<*static_cast<fmatvec::Vector<fmatvec::Var, fmatvec::IndependentVariable>*>(getSwigPtr(c(i)));
    else
      throw runtime_error("Unknown type for independent variable in function: "+type);
  }
  str<<")=";
  string type=getSwigType(c(nrIndeps));
  auto cc=C(c(nrIndeps));
  if(valueIsOfType(cc, ScalarType))
    str<<fmatvec::SymbolicExpression(cast<double>(cc));
  else if(valueIsOfType(cc, VectorType))
    str<<static_cast<fmatvec::Vector<fmatvec::Var, fmatvec::SymbolicExpression>>(
              fmatvec::VecV(cast<vector<double>>(cc)));
  else if(valueIsOfType(cc, MatrixType))
    str<<static_cast<fmatvec::Matrix<fmatvec::General, fmatvec::Var, fmatvec::Var, fmatvec::SymbolicExpression>>(
              fmatvec::MatV(cast<vector<vector<double>>>(cc)));
  else if(type=="SymbolicExpression" || type=="IndependentVariable")
    str<<*static_cast<fmatvec::SymbolicExpression*>(getSwigPtr(c(nrIndeps)));
  else if(type=="VectorSym" || type=="VectorIndep")
    str<<*static_cast<fmatvec::Vector<fmatvec::Var, fmatvec::SymbolicExpression>*>(getSwigPtr(c(nrIndeps)));
  else if(type=="MatrixSym")
    str<<*static_cast<fmatvec::Matrix<fmatvec::General, fmatvec::Var, fmatvec::Var, fmatvec::SymbolicExpression>*>(getSwigPtr(c(nrIndeps)));
  else
    throw runtime_error("Unknown type for dependent variable in function: "+type);
  return str.str();
}

OctEval::OctEval(vector<bfs::path> *dependencies_) : Eval(dependencies_) {
  auto ci=make_shared<Import>();
  currentImport=ci;
  ci->path=octInit.initialPath;

  // add .../share/mbxmlutils/octave to octave search path (MBXMLUtils m-files are stored their)
  addImportHelper(installPath/"share"/"mbxmlutils"/"octave");
};

OctEval::~OctEval() = default;

Eval::Value OctEval::createFunctionIndep(int dim) const {
  if(dim==0)
    return createSwigByTypeName("IndependentVariable");
  auto ret=createSwigByTypeName("VectorIndep");
  auto vec=static_cast<fmatvec::Vector<fmatvec::Var, fmatvec::IndependentVariable>*>(getSwigPtr(*C(ret)));
  vec->resize(dim, fmatvec::NONINIT);
  return ret;
}

void OctEval::addImportHelper(const boost::filesystem::path &dir) {
  // some special handing for the octave addpath is required since addpath is very time consuming
  // in octave. Hence we try to change the path as less as possible. See also fullStringToValue.

  static octave_function *addpath=octInit.interpreter->get_symbol_table().find_function("addpath").function_value(); // get ones a pointer for performance reasons
  static octave_function *path=octInit.interpreter->get_symbol_table().find_function("path").function_value(); // get ones a pointer for performance reasons
  // set octave path to top of stack of not already done
  string curPath;
  curPath=fevalThrow(path, octave_value_list(), 1)(0).string_value();
  auto ci=static_pointer_cast<Import>(currentImport);
  string &currentPath=ci->path;
  if(curPath!=currentPath)
  {
    // set path
    fevalThrow(path, octave_value_list(octave_value(currentPath)), 0,
      "Unable to set the octave search path "+currentPath);
  }
  // add dir to octave path
  auto vn1=octInit.interpreter->get_symbol_table().variable_names();
  //auto gvn1=octInit.interpreter->get_symbol_table().global_variable_names();
  //auto ufn1=octInit.interpreter->get_symbol_table().user_function_names();
  //auto tlvn1=octInit.interpreter->get_symbol_table().top_level_variable_names();
  fevalThrow(addpath, octave_value_list(octave_value(absolute(dir).string())), 0,
    "Unable to add octave search path "+absolute(dir).string());
  auto vn2=octInit.interpreter->get_symbol_table().variable_names();
  //auto gvn2=octInit.interpreter->get_symbol_table().global_variable_names();
  //auto ufn2=octInit.interpreter->get_symbol_table().user_function_names();
  //auto tlvn2=octInit.interpreter->get_symbol_table().top_level_variable_names();
  // get new path and store it in top of stack
  currentPath=fevalThrow(path, octave_value_list(), 1)(0).string_value();

  // create a list of all variables added by the addPath command and register these as parameter
  // to restore it in any new context with this addPath.
  auto fillVars=[](const list<string> &l1, const list<string> &l2, map<string, octave_value> &im,
                      function<octave_value(const string&)> get){
    set<string> s1(l1.begin(), l1.end());
    set<string> s2(l2.begin(), l2.end());
    set<string> newVars;
    set_difference(l2.begin(), l2.end(), l1.begin(), l1.end(), inserter(newVars, newVars.begin()));
    for(auto &n : newVars)
      im[n]=get(n);
  };
  auto &gst=octInit.interpreter->get_symbol_table();
  fillVars(vn1  , vn2  , ci->vn  , bind(&symbol_table::varval            , &gst, placeholders::_1));
//fillVars(gvn1 , gvn2 , ci->gvn , bind(&symbol_table::global_varval     , &gst, placeholders::_1));
//fillVars(ufn1 , ufn2 , ci->ufn , bind(&symbol_table::find_user_function, &gst, placeholders::_1));
//fillVars(tlvn1, tlvn2, ci->tlvn, bind(&symbol_table::top_level_varval  , &gst, placeholders::_1));
}

void OctEval::addImport(const string &code, const DOMElement *e) {
  try {
    bfs::path dir;
    if(e) {
      // evaluate code to and string (directory to add using addpath)
      dir=cast<string>(fullStringToValue(code, e));
      // convert to an absolute path using e
      dir=E(e)->convertPath(dir);
    }
    else
      dir=code;

    addImportHelper(dir);

    if(!dependencies)
      return;
    // add m-files in dir to dependencies
    for(bfs::directory_iterator it=bfs::directory_iterator(dir); it!=bfs::directory_iterator(); it++)
      if(it->path().extension()==".m")
        dependencies->push_back(it->path());
  } RETHROW_AS_DOMEVALEXCEPTION(e)
}

Eval::Value OctEval::fullStringToValue(const string &str, const DOMElement *e) const {
  // check some common string to avoid time consiming evaluation
  // check true and false
  if(str=="true") return make_shared<octave_value>(1);
  if(str=="false") return make_shared<octave_value>(0);
  // check for floating point values
  double d;
  char *end;
  d=strtod(str.c_str(), &end);
  if(end!=str && boost::algorithm::trim_copy(string(end))=="")
    return make_shared<octave_value>(d);
  // no common string detected -> evaluate using octave now

  // restore current dir on exit and change current dir
  PreserveCurrentDir preserveDir;
  if(e) {
    bfs::path chdir=E(e)->getOriginalFilename().parent_path();
    if(!chdir.empty())
      bfs::current_path(chdir);
  }

  // clear octave variables
  octInit.interpreter->get_symbol_table().current_scope().clear_variables();
  // restore current parameters
  for(const auto & i : currentParam)
    octInit.interpreter->get_symbol_table().assign(i.first, *C(i.second));

  // change the octave serach path only if required (for performance reasons; addpath/path(...) is very time consuming, but not path())
  static octave_function *path=octInit.interpreter->get_symbol_table().find_function("path").function_value(); // get ones a pointer for performance reasons
  string curPath;
  try { curPath=fevalThrow(path, octave_value_list(), 1)(0).string_value(); } RETHROW_AS_DOMEVALEXCEPTION(e)
  auto ci=static_pointer_cast<Import>(currentImport);
  string &currentPath=ci->path;
  if(curPath!=currentPath)
  {
    // set path
    try { fevalThrow(path, octave_value_list(octave_value(currentPath)), 0,
      "Unable to set the octave search path "+currentPath); } RETHROW_AS_DOMEVALEXCEPTION(e)
  }

  // restore variables from import
  auto restoreVars=[](map<string, octave_value> &im, function<void(const string&, const octave_value &v)> set) {
    for(auto &i : im)
      set(i.first, i.second);
  };
  auto &gst=octInit.interpreter->get_symbol_table();
  using GstFuncType = void(symbol_table::*)(const string&, const octave_value &);
  restoreVars(ci->vn  , bind<GstFuncType>(&symbol_table::assign               , &gst, std::placeholders::_1, std::placeholders::_2));
//restoreVars(ci->gvn , bind<GstFuncType>(&symbol_table::global_assign        , &gst, placeholders::_1, placeholders::_2));
//restoreVars(ci->ufn , bind<GstFuncType>(&symbol_table::install_user_function, &gst, placeholders::_1, placeholders::_2));
//restoreVars(ci->tlvn, bind<GstFuncType>(&symbol_table::top_level_assign     , &gst, placeholders::_1, placeholders::_2));

  ostringstream err;
  ostringstream out;
  try{
    int dummy;
    REDIR_STDOUT(out.rdbuf());
    REDIR_STDERR(err.rdbuf());
    mbxmlutilsStaticDependencies.clear();
    octInit.interpreter->eval_string(str, true, dummy, 0); // eval as statement list
    addStaticDependencies(e);
  }
  catch(const exception &ex) { // should not happend
    error_state=0;
    throw DOMEvalException(string(ex.what())+":\n"+out.str()+"\n"+err.str(), e);
  }
  catch(...) { // should not happend
    error_state=0;
    throw DOMEvalException("Unknwon exception:\n"+out.str()+"\n"+err.str(), e);
  }
  if(error_state!=0) { // if error => wrong code => throw error
    error_state=0;
    throw DOMEvalException(out.str()+"\n"+err.str()+"Unable to evaluate expression: "+str, e);
  }
  // generate a strNoSpace from str by removing leading/trailing spaces as well as trailing ';'.
  string strNoSpace=str;
  while(!strNoSpace.empty() && (strNoSpace[0]==' ' || strNoSpace[0]=='\n'))
    strNoSpace=strNoSpace.substr(1);
  while(!strNoSpace.empty() && (strNoSpace[strNoSpace.size()-1]==' ' || strNoSpace[strNoSpace.size()-1]==';' ||
    strNoSpace[strNoSpace.size()-1]=='\n'))
    strNoSpace=strNoSpace.substr(0, strNoSpace.size()-1);
  if(!octInit.interpreter->get_symbol_table().current_scope().is_variable("ret") && !octInit.interpreter->get_symbol_table().current_scope().is_variable("ans") && !octInit.interpreter->get_symbol_table().current_scope().is_variable(strNoSpace)) {
    throw DOMEvalException("'ret' variable not defined in multi statement octave expression or incorrect single statement: "+
      str, e);
  }
  octave_value ret;
  if(octInit.interpreter->get_symbol_table().current_scope().is_variable(strNoSpace))
    ret=octInit.interpreter->get_symbol_table().varval(strNoSpace);
  else if(!octInit.interpreter->get_symbol_table().current_scope().is_variable("ret"))
    ret=octInit.interpreter->get_symbol_table().varval("ans");
  else
    ret=octInit.interpreter->get_symbol_table().varval("ret");

  return C(ret);
}

bool OctEval::valueIsOfType(const Value &value, OctEval::ValueType type) const {
  shared_ptr<octave_value> v=C(value);
  switch(type) {
    case ScalarType:
      if(!v->is_string() && v->is_scalar_type() && v->isreal()) return true;
      return false;

    case VectorType:
      if(valueIsOfType(value, ScalarType)) return true;
      if(!v->is_string() && v->is_matrix_type() && v->isreal()) {
        Matrix m=v->matrix_value();
        if(m.cols()==1) return true;
      }
      return false;

    case MatrixType:
      if(valueIsOfType(value, ScalarType)) return true;
      if(valueIsOfType(value, VectorType)) return true;
      if(!v->is_string() && v->is_matrix_type() && v->isreal()) return true;
      return false;

    case StringType:
      if(v->is_string()) return true;
      return false;

    case FunctionType:
      if(v->iscell()) return true;
      return false;
  }
  return false;
}

octave_value_list OctEval::fevalThrow(octave_function *func, const octave_value_list &arg, int n,
                                       const string &msg) {
  ostringstream out;
  ostringstream err;
  octave_value_list ret;
  {
    REDIR_STDOUT(out.rdbuf());
    REDIR_STDERR(err.rdbuf());
    ret=feval(func, arg, n);
  }
  if(error_state!=0) {
    error_state=0;
    throw runtime_error(msg+":\n"+out.str()+"\n"+err.str());
  }
  return ret;
}

map<bfs::path, pair<bfs::path, bool> >& OctEval::requiredFiles() const {
  static map<bfs::path, pair<bfs::path, bool> > files;
  if(!files.empty())
    return files;

  fmatvec::Atom::msgStatic(Info)<<"Generate file list for MBXMLUtils m-files."<<endl;
  for(bfs::directory_iterator srcIt=bfs::directory_iterator(installPath/"share"/"mbxmlutils"/"octave");
    srcIt!=bfs::directory_iterator(); ++srcIt) {
    if(is_directory(*srcIt)) // skip directories
      continue;
    files[srcIt->path()]=make_pair(bfs::path("share")/"mbxmlutils"/"octave", false);
  }

  files[installPath/LIBDIR/"fmatvec_symbolic_swig_octave.oct"]=make_pair(LIBDIR, true);
  files[installPath/LIBDIR/"registerPath.oct"]=make_pair(LIBDIR, true);

  // get octave fcnfiledir without octave_prefix
  bfs::path octave_fcnfiledir(config::fcn_file_dir().substr(config::octave_home().length()+1));
  fmatvec::Atom::msgStatic(Info)<<"Generate file list for octave m-files."<<endl;
  bfs::path dir=config::octave_home()/octave_fcnfiledir;
  size_t depth=distance(dir.begin(), dir.end());
  for(bfs::recursive_directory_iterator srcIt=bfs::recursive_directory_iterator(dir);
      srcIt!=bfs::recursive_directory_iterator(); ++srcIt) {
    if(is_directory(*srcIt)) // skip directories
      continue;
    bfs::path::iterator dstIt=srcIt->path().begin();
    for(int i=0; i<depth; ++i) ++dstIt;
    bfs::path dst;
    for(; dstIt!=--srcIt->path().end(); ++dstIt)
      dst/=*dstIt;
    files[srcIt->path()]=make_pair(octave_fcnfiledir/dst, false);
  }

  // get octave octfiledir without octave_prefix
  bfs::path octave_octfiledir(config::oct_file_dir().substr(config::octave_home().length()+1));
  fmatvec::Atom::msgStatic(Info)<<"Generate file list for octave oct-files (excluding dependencies)."<<endl;
  for(bfs::directory_iterator srcIt=bfs::directory_iterator(config::octave_home()/octave_octfiledir);
      srcIt!=bfs::directory_iterator(); ++srcIt) {
    if(srcIt->path().filename()=="__init_gnuplot__.oct") continue;
    if(srcIt->path().extension()==".oct")
      files[srcIt->path()]=make_pair(octave_octfiledir, true);
    if(srcIt->path().filename()=="PKG_ADD")
      files[srcIt->path()]=make_pair(octave_octfiledir, false);
  }

  return files;
}

Eval::Value OctEval::callFunction(const string &name, const vector<Value>& args) const {
  static map<string, octave_function*> functionValue;
  pair<map<string, octave_function*>::iterator, bool> f=functionValue.insert(make_pair(name, static_cast<octave_function*>(nullptr)));
  if(f.second)
    f.first->second=octInit.interpreter->get_symbol_table().find_function(name).function_value(); // get ones a pointer performance reasons
  octave_value_list octargs;
  for(const auto & arg : args)
    octargs.append(*C(arg));
  octave_value_list ret=fevalThrow(f.first->second, octargs, 1,
    "Unable to call function "+name+".");
  return C(ret(0));
}

} // end namespace MBXMLUtils

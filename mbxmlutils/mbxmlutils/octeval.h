#ifndef _MBXMLUTILS_OCTEVAL_H_
#define _MBXMLUTILS_OCTEVAL_H_

#include "eval.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

class octave_value;
class octave_value_list;
class octave_function;

namespace XERCES_CPP_NAMESPACE { class DOMElement; }

namespace MBXMLUtils {

boost::shared_ptr<octave_value> C(const boost::shared_ptr<void> &value);

boost::shared_ptr<void> C(const octave_value &value);

class OctEval;

/*! Octave expression evaluator and converter. */
class OctEval : public Eval {
  friend class Eval;

  protected:
    //! Constructor.
    OctEval(std::vector<boost::filesystem::path> *dependencies_=NULL);

  public:
    //! Destructor.
    ~OctEval();

    //! Get the name of this evaluator.
    static std::string getNameStatic() { return "octave"; }
    std::string getName() { return getNameStatic(); }

    //! Add dir to octave search path
    //! A relative path in dir is expanded to an absolute path using the current directory.
    void addPath(const boost::filesystem::path &dir, const xercesc::DOMElement *e);

    //! get the type of value
    bool valueIsOfType(const boost::shared_ptr<void> &value, ValueType type);

    //! return a list of all required files of octave (excluding dependent files of libraries)
    std::map<boost::filesystem::path, std::pair<boost::filesystem::path, bool> >& requiredFiles();

  protected:

    //! This function deinitialized octave. It is used in the dtor and before exceptions in the ctor are thrown
    // (in the later case the dtor is not called but octave must be uninitialized before exit)
    void deinitOctave();

    //! evaluate str fully and return result as an octave variable
    virtual boost::shared_ptr<void> fullStringToValue(const std::string &str, const xercesc::DOMElement *e=NULL);

    //! get the SWIG pointer of this value.
    void* getSwigThis(const boost::shared_ptr<void> &value);

    //! get the SWIG class name of this value.
    std::string getSwigType(const boost::shared_ptr<void> &value);

    boost::shared_ptr<void> createSwigByTypeName(const std::string &typeName);

    // initial path
    static std::string initialPath;
    static std::string pathSep;

    static int initCount;

    static octave_value_list fevalThrow(octave_function *func, const octave_value_list &arg, int n=0,
                                        const std::string &msg=std::string());

    virtual boost::shared_ptr<void> callFunction(const std::string &name, const std::vector<boost::shared_ptr<void> >& args);

    virtual double                            cast_double              (const boost::shared_ptr<void> &value);
    virtual std::vector<double>               cast_vector_double       (const boost::shared_ptr<void> &value);
    virtual std::vector<std::vector<double> > cast_vector_vector_double(const boost::shared_ptr<void> &value);
    virtual std::string                       cast_string              (const boost::shared_ptr<void> &value);

    virtual boost::shared_ptr<void> create_double              (const double& v);
    virtual boost::shared_ptr<void> create_vector_double       (const std::vector<double>& v);
    virtual boost::shared_ptr<void> create_vector_vector_double(const std::vector<std::vector<double> >& v);
    virtual boost::shared_ptr<void> create_string              (const std::string& v);

  private:
    static std::map<std::string, octave_function*> functionValue;
};

} // end namespace MBXMLUtils

#endif

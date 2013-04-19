/* 
 * File:   search.h
 * Author: savop
 *
 * Created on 27. August 2009, 21:21
 */

#ifndef _SEARCH_H
#define	_SEARCH_H

#include <map>
#include <vdr/tools.h>
#include "util.h"

/**
 * Sort criteria
 *
 * This is a structure for sorting objects. It has a certain property and
 * a direction flag.
 */
struct cSortCrit : public cListObject {
    const char* Property;               ///< the Property, which shall be sorted
    bool SortDescending;                ///< sort the objects in descending order
};

typedef std::map<const char*, const char*, strCmp> propertyMap;

/**
 * Web path parser
 *
 * Parses paths which came from the webserver. It splits the path into
 * a section, a certain method and its properties.
 *
 * This can be used to easily determine which file was requested by a client
 */
class cPathParser {
private:
    cString             mKey;
    propertyMap         mProperties;
    int                 mSection;
    int                 mMethod;
    bool parsePath(const char* Path, int* Section, int* Method, propertyMap* Properties);
    void pushPropertyKey(const char* Start, const char* End);
    void pushPropertyValue(const char* Start, const char* End);
    void pushMethod(int Method);
    void pushSection(int Section);
    cPathParser();
public:
    virtual ~cPathParser();
    /**
     * Parses the path
     *
     * This will parse the path and stores the result in the pointers given.
     *
     * @return returns
     * - \bc true, if the parsing was successful
     * - \bc false, otherwise
     */
    static bool parse(
        const char* Path,           ///< the path which is parsed
        int* Section,               ///< the number of the registered section
        int* Method,                ///< the number of the registered method
        propertyMap* Properties     ///< the properties found in the path
    );
};

/**
 * Creates a list with sort criteria
 *
 * This parser creates a list of sort criteria. It parses the sort criteria string
 * from a \em Browse or \em Search request and stores the information in a \c cSortCrit
 * structure.
 */
class cSortCriteria {
private:
    cSortCrit*        mCurrentCrit;
    cList<cSortCrit>* mCriteriaList;
    bool parseSort(const char* Sort);
    void pushProperty(const char* Property);
    void pushDirection(const char Direction);
    cList<cSortCrit>* getSortList() const { return this->mCriteriaList; }
    cSortCriteria();
public:
    virtual ~cSortCriteria();
    /**
     * Parses the sort criteria
     *
     * This parses the sort criteria and returns a list with valid criterias
     *
     * @return returns
     * - a list with valid sort criterias
     * - \bc null, otherwise
     */
    static cList<cSortCrit>* parse(
        const char* Sort            ///< the string container the sort criteria
    );
};

/**
 * Parses the filter criteria
 *
 * This parses the filter criteria which comes from a \em Browse or \em Search
 * request.
 */
class cFilterCriteria {
private:
    cStringList* mFilterList;
    cFilterCriteria();
    bool parseFilter(const char* Filter);
    void pushProperty(const char* Property);
    void pushAsterisk(const char Asterisk);
    cStringList* getFilterList() const { return this->mFilterList; }
public:
    virtual ~cFilterCriteria();
    /**
     * Parses the filter criteria
     *
     * This parses the filter criteria. It may be a empty string list, a \bc NULL
     * pointer or a list with properties which shall be shown in the \em DIDL-Lite fragment.
     *
     * @return the stringlist containing the filter
     */
    static cStringList* parse(
        const char* Filter          ///< the filter string
    );
};

/**
 * @private
 * @todo This is implemented very soon
 */
class cSearch {
private:
    char* SQLWhereStmt;
    const char* CurrentProperty;
    const char* CurrentOperator;
    const char* CurrentValue;
    static cSearch* mInstance;
    cSearch();
    bool parseCriteria(const char* Search);
    void pushExistance (const char* Exists);
    void pushProperty  (const char* Property);
    void pushOperator  (const char* Operator);
    void pushConcatOp  (const char* Operator);
    void pushStartBrackedExp(const char);
    void pushEndBrackedExp(const char);
    void pushValue     (const char* Start, const char* End);
    void pushExpression(const char* Start, const char* End);
public:
    virtual ~cSearch();
    static const char* parse(const char* Search);
};


#endif	/* _SEARCH_H */


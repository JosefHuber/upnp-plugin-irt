/*
 * File:   search.cpp
 * Author: savop
 *
 * Created on 27. August 2009, 21:21
 */

// uncomment this to enable debuging of the grammar
//#define BOOST_SPIRIT_DEBUG

#include <string>
#include "search.h"
#include "../common.h"
#include <boost/spirit.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace std;
using namespace boost;
using namespace boost::spirit;

// This is the standard callback function which will be overloaded
// with all the specific callbacks
typedef function2<void, const char*, const char*> expCallback;
typedef function1<void, const char*> propCallback;
typedef function1<void, const char*> opCallback;
typedef function1<void, const char> charCallback;
typedef function1<void, int> intCallback;

// The defined ColumnNames
/** @private */
struct cProperties : symbols<const char*> {
//struct cProperties : symbols<> {
    cProperties(){
        add
//                (UPNP_PROP_OBJECTID, UPNP_PROP_OBJECTID)
//                (UPNP_PROP_PARENTID, UPNP_PROP_PARENTID)
//                (UPNP_PROP_RESTRICTED, UPNP_PROP_RESTRICTED)
//                (UPNP_PROP_CLASS, UPNP_PROP_CLASS)
//                (UPNP_PROP_CLASSNAME, UPNP_PROP_CLASSNAME)
//                (UPNP_PROP_WRITESTATUS, UPNP_PROP_WRITESTATUS)
//                (UPNP_PROP_CHILDCOUNT, UPNP_PROP_CHILDCOUNT)
//                (UPNP_PROP_REFERENCEID, UPNP_PROP_REFERENCEID)
//                (UPNP_PROP_TITLE, UPNP_PROP_TITLE)
//                (UPNP_PROP_CREATOR, UPNP_PROP_CREATOR)
//                ("dc:description", "Description")
//                ("dc:date", "Date")
//                ("res", "Resource")
//                ("res@bitrate", "Bitrate")
//                ("res@duration", "Duration")
//                ("res@size", "Size")
//                ("res@sampleFrequency", "SampleFrequency")
//                ("res@resolution", "Resolution")
//                ("res@protocolInfo", "ProtocolInfo")
//                ;
                (UPNP_PROP_OBJECTID,UPNP_PROP_OBJECTID)
                (UPNP_PROP_PARENTID,UPNP_PROP_PARENTID)
                (UPNP_PROP_TITLE,UPNP_PROP_TITLE)
                (UPNP_PROP_CREATOR,UPNP_PROP_CREATOR)
                (UPNP_PROP_RESTRICTED,UPNP_PROP_RESTRICTED)
                (UPNP_PROP_WRITESTATUS,UPNP_PROP_WRITESTATUS)
                (UPNP_PROP_CLASS,UPNP_PROP_CLASS)
                (UPNP_PROP_CLASSNAME,UPNP_PROP_CLASSNAME)
                (UPNP_PROP_SEARCHCLASS,UPNP_PROP_SEARCHCLASS)
                (UPNP_PROP_SCLASSDERIVED,UPNP_PROP_SCLASSDERIVED)
                (UPNP_PROP_REFERENCEID,UPNP_PROP_REFERENCEID)
                (UPNP_PROP_SCLASSNAME,UPNP_PROP_SCLASSNAME)
                (UPNP_PROP_SEARCHABLE,UPNP_PROP_SEARCHABLE)
                (UPNP_PROP_CHILDCOUNT,UPNP_PROP_CHILDCOUNT)
                (UPNP_PROP_RESOURCE,UPNP_PROP_RESOURCE)
                (UPNP_PROP_PROTOCOLINFO,UPNP_PROP_PROTOCOLINFO)
                (UPNP_PROP_SIZE,UPNP_PROP_SIZE)
                (UPNP_PROP_DURATION,UPNP_PROP_DURATION)
                (UPNP_PROP_BITRATE,UPNP_PROP_BITRATE)
                (UPNP_PROP_SAMPLEFREQUENCE,UPNP_PROP_SAMPLEFREQUENCE)
                (UPNP_PROP_BITSPERSAMPLE,UPNP_PROP_BITSPERSAMPLE)
                (UPNP_PROP_NOAUDIOCHANNELS,UPNP_PROP_NOAUDIOCHANNELS)
                (UPNP_PROP_COLORDEPTH,UPNP_PROP_COLORDEPTH)
                (UPNP_PROP_RESOLUTION,UPNP_PROP_RESOLUTION)
                (UPNP_PROP_GENRE,UPNP_PROP_GENRE)
                (UPNP_PROP_LONGDESCRIPTION,UPNP_PROP_LONGDESCRIPTION)
                (UPNP_PROP_PRODUCER,UPNP_PROP_PRODUCER)
                (UPNP_PROP_RATING,UPNP_PROP_RATING)
                (UPNP_PROP_ACTOR,UPNP_PROP_ACTOR)
                (UPNP_PROP_DIRECTOR,UPNP_PROP_DIRECTOR)
                (UPNP_PROP_DESCRIPTION,UPNP_PROP_DESCRIPTION)
                (UPNP_PROP_PUBLISHER,UPNP_PROP_PUBLISHER)
                (UPNP_PROP_LANGUAGE,UPNP_PROP_LANGUAGE)
                (UPNP_PROP_RELATION,UPNP_PROP_RELATION)
                (UPNP_PROP_STORAGEMEDIUM,UPNP_PROP_STORAGEMEDIUM)
                (UPNP_PROP_DVDREGIONCODE,UPNP_PROP_DVDREGIONCODE)
                (UPNP_PROP_CHANNELNAME,UPNP_PROP_CHANNELNAME)
                (UPNP_PROP_SCHEDULEDSTARTTIME,UPNP_PROP_SCHEDULEDSTARTTIME)
                (UPNP_PROP_SCHEDULEDENDTIME,UPNP_PROP_SCHEDULEDENDTIME)
                (UPNP_PROP_ICON,UPNP_PROP_ICON)
                (UPNP_PROP_REGION,UPNP_PROP_REGION)
                (UPNP_PROP_CHANNELNR,UPNP_PROP_CHANNELNR)
                (UPNP_PROP_RIGHTS,UPNP_PROP_RIGHTS)
                (UPNP_PROP_RADIOCALLSIGN,UPNP_PROP_RADIOCALLSIGN)
                (UPNP_PROP_RADIOSTATIONID,UPNP_PROP_RADIOSTATIONID)
                (UPNP_PROP_RADIOBAND,UPNP_PROP_RADIOBAND)
                (UPNP_PROP_CONTRIBUTOR,UPNP_PROP_CONTRIBUTOR)
                (UPNP_PROP_DATE,UPNP_PROP_DATE)
                (UPNP_PROP_ALBUM,UPNP_PROP_ALBUM)
                (UPNP_PROP_ARTIST,UPNP_PROP_ARTIST)
                (UPNP_PROP_DLNA_CONTAINERTYPE,UPNP_PROP_DLNA_CONTAINERTYPE)
                ;
    }
} Properties;

/** @private */
struct cOperators : symbols<const char*> {
    cOperators(){
        add
                ("=", "==")
                ("!=", "!=")
                ("<", "<")
                (">", ">")
                ("<=", "<=")
                (">=", ">=")
                ("contains", "LIKE")
                ("doesNotContain", "NOT LIKE")
                ("derivedfrom", "derivedFrom")
                ;
    }
} Operators;

/** @private */
struct cConcatOperators : symbols<const char*> {
    cConcatOperators(){
        add
                ("and", "AND")
                ("or", "OR")
                ;
    }
} ConcatOperators;

/** @private */
struct cExistanceOperator : symbols<const char*> {
    cExistanceOperator(){
        add
                ("true", "NOT NULL")
                ("false", "NULL")
                ;
    }
} Existance;

// THE GRAMMAR!
// This is the grammar including the functors which calls the member functions
// of search. The callback definitions at the end of the constructor are
// essential. DO NOT MODIFY if you don't know how!
/** @private */
struct cSearchGrammar : public boost::spirit::grammar<cSearchGrammar> {
    // The callbacks members
    charCallback &endBrackedExp;
    expCallback &pushSimpleExp;
    opCallback  &pushOperator;
    expCallback &pushQuotedValue;
    opCallback  &pushExistance;
    propCallback &pushProperty;
    opCallback &pushConcatOp;
    charCallback  &startBrackedExp;

    // Constructor with the callback functions
    cSearchGrammar(
            charCallback &endBrackedExp,
            expCallback &pushSimpleExp,
            opCallback &pushOperator,
            expCallback &pushQuotedValue,
            opCallback &pushExistance,
            propCallback &pushProperty,
            opCallback &pushConcatOp,
            charCallback  &startBrackedExp):
                    endBrackedExp(endBrackedExp),
                    pushSimpleExp(pushSimpleExp),
                    pushOperator(pushOperator),
                    pushQuotedValue(pushQuotedValue),
                    pushExistance(pushExistance),
                    pushProperty(pushProperty),
                    pushConcatOp(pushConcatOp),
                    startBrackedExp(startBrackedExp){}

    template <typename scanner>
    /** @private */
    struct definition {
        boost::spirit::rule<scanner> searchCrit, searchExp, logOp, \
                                     relExp, binOp, relOp, stringOp, \
                                     existsOp, boolVal, quotedVal, \
                                     wChar, property, brackedExp, exp;
        const boost::spirit::rule<scanner> &start(){
            return searchCrit;
        }
        definition(const cSearchGrammar &self){
            /*************************************************************************\
            *                                                                         *
            *                   The grammar of a UPnP search expression               *
            *                                                                         *
            * searchCrit        ::= searchExp | asterisk                              *
            *                                                                         *
            * searchExp         ::= relExp |                                          *
            *                       searchExp wChar+ logOp wChar+ searchExp |         *
            *                       '(' wChar* searchExp wChar* ')'                   *
            *                                                                         *
            * logOp             ::= 'and' | 'or'                                      *
            *                                                                         *
            * relExp            ::= property wChar+ binOp wChar+ quotedVal |          *
            *                       property wChar* existsOp wChar+ boolVal           *
            *                                                                         *
            * binOp             ::= relOp | stringOp                                  *
            *                                                                         *
            * relOp             ::= '=' | '!=' | '<' | '<=' | '>' | '>='              *
            *                                                                         *
            * stringOp          ::= 'contains' | 'doesNotContain' | 'derivedfrom'     *
            *                                                                         *
            * existsOp          ::= 'exists'                                          *
            *                                                                         *
            * boolVal           ::= 'true' | 'false'                                  *
            *                                                                         *
            * quotedVal         ::= dQuote escapedQuote dQuote                        *
            *                                                                         *
            * wChar             ::= space | hTab | lineFeed |                         *
            *                       vTab | formFeed | return                          *
            *                                                                         *
            * property          ::= See ContentDirectory Section 2.4                  *
            *                                                                         *
            * escapedQuote      ::= See ContentDirectory Section 2.3.1                *
            *                                                                         *
            \*************************************************************************/
            searchCrit  = searchExp | "*";

            searchExp   = exp >> *(+wChar >> logOp >> +wChar >> exp);
                          ;

            exp         = relExp
                          | brackedExp
                          ;

            brackedExp  = confix_p(
                                ch_p('(')[self.startBrackedExp],
                                *wChar >> searchExp >> *wChar,
                                ch_p(')')[self.endBrackedExp]
                          )
                          ;

            logOp       = ConcatOperators[self.pushConcatOp]
                          ;

            relExp      = (property >> +wChar >> binOp >> +wChar >> quotedVal) [self.pushSimpleExp]
                          | (property >> +wChar >> existsOp >> +wChar >> boolVal) [self.pushSimpleExp]
                          ;

            binOp       = Operators[self.pushOperator]
                          ;

            existsOp    = str_p("exists")
                          ;

            boolVal     = Existance[self.pushExistance]
                          ;

            quotedVal   = confix_p('"', (*c_escape_ch_p)[self.pushQuotedValue], '"');

            wChar       = space_p;

            property    = Properties[self.pushProperty];

            // Debug mode
            #ifdef BOOST_SPIRIT_DEBUG
                BOOST_SPIRIT_DEBUG_NODE(searchCrit);
                BOOST_SPIRIT_DEBUG_NODE(searchExp);
                BOOST_SPIRIT_DEBUG_NODE(logOp);
                BOOST_SPIRIT_DEBUG_NODE(relExp);
                BOOST_SPIRIT_DEBUG_NODE(binOp);
                BOOST_SPIRIT_DEBUG_NODE(relOp);
                BOOST_SPIRIT_DEBUG_NODE(stringOp);
                BOOST_SPIRIT_DEBUG_NODE(existsOp);
                BOOST_SPIRIT_DEBUG_NODE(boolVal);
                BOOST_SPIRIT_DEBUG_NODE(quotedVal);
                BOOST_SPIRIT_DEBUG_NODE(wChar);
                BOOST_SPIRIT_DEBUG_NODE(property);
            #endif
        }
    };
};

/** @private */
struct cSortGrammar : public boost::spirit::grammar<cSortGrammar> {
    // The callback members
    propCallback &pushProperty;
    charCallback &pushDirection;

    cSortGrammar(
            propCallback &pushProperty,
            charCallback &pushDirection):
            pushProperty(pushProperty),
            pushDirection(pushDirection){}

    template <typename scanner>
    /** @private */
    struct definition {
        boost::spirit::rule<scanner> sortCrit, sortExp, property, direction;

        const boost::spirit::rule<scanner> &start(){
            return sortCrit;
        }
        definition(const cSortGrammar &self){
            sortCrit   = sortExp
                         ;

            sortExp    = direction >> property >> *(ch_p(',') >> sortExp)
                         ;

            direction  = sign_p[self.pushDirection]
                         ;

            property   = Properties[self.pushProperty]
                         ;
        }
    };
};

/** @private */
struct cFilterGrammar : public boost::spirit::grammar<cFilterGrammar> {
    // The callback members
    propCallback &pushProperty;
    charCallback &pushAsterisk;

    cFilterGrammar(
            propCallback &pushProperty,
            charCallback &pushAsterisk):
            pushProperty(pushProperty),
            pushAsterisk(pushAsterisk){}

    template <typename scanner>
    /** @private */
    struct definition {
        boost::spirit::rule<scanner> filterCrit, filterExp, property;

        const boost::spirit::rule<scanner> &start(){
            return filterCrit;
        }
        definition(const cFilterGrammar &self){
            filterCrit = filterExp
                         | ch_p('*')[self.pushAsterisk]
                         ;

            filterExp  = property >> *(ch_p(',') >> filterExp)
                         ;

            property   = Properties[self.pushProperty]
                         ;
        }
    };
};

 /**********************************************\
 *                                              *
 *  The actors                                  *
 *                                              *
 \**********************************************/

void cSearch::pushEndBrackedExp(const char){
    MESSAGE(VERBOSE_PARSERS, "Pushing closing bracket");
    if(asprintf(&this->SQLWhereStmt, "%s)", this->SQLWhereStmt)==-1){
        ERROR("Unable to allocate SQL Statement");
        return;
    }
}

void cSearch::pushExpression(const char*, const char*){

    const char* Property = this->CurrentProperty;
    const char* Operator = this->CurrentOperator;
    const char* Value    = this->CurrentValue;

    if(Property && Operator && Value){
        char* Statement;
        long int IntegerValue;

        if(sscanf(Value, "%ld", &IntegerValue)!=EOF && sscanf(Value, "%*4d-%*2d-%*2d")==EOF){
            MESSAGE(VERBOSE_PARSERS, "Popping '%s %s %ld'",Property, Operator, IntegerValue);
            if(asprintf(&Statement, "%s %s %ld", Property, Operator, IntegerValue)==-1){
                ERROR("Failed to allocated memory for statement.");
                return;
            }
        }
        else if(!strcasecmp(Operator, "IS")){
            MESSAGE(VERBOSE_PARSERS, "Popping '%s %s %s'", Property, Operator, Value);
            if(asprintf(&Statement, "%s %s %s", Property, Operator, Value)==-1){
                ERROR("Failed to allocated memory for statement.");
                return;
            }
        }
        else {
            MESSAGE(VERBOSE_PARSERS, "Popping '%s %s \"%s\"'",Property, Operator, Value);
            if(asprintf(&Statement, "%s %s '%s'", Property, Operator, Value)==-1){
                ERROR("Failed to allocated memory for statement.");
                return;
            }
        }

        if(asprintf(&this->SQLWhereStmt, "%s %s", this->SQLWhereStmt, Statement)==-1){
            ERROR("Unable to allocate SQL Statement");
            return;
        }

    }
    return;
}

void cSearch::pushProperty(const char* Property){
    this->CurrentProperty = strdup(Property);
    MESSAGE(VERBOSE_PARSERS, "Property %s added",Property);
}

void cSearch::pushOperator(const char* Operator){
    this->CurrentOperator = strdup(Operator);
    MESSAGE(VERBOSE_PARSERS, "Operator %s added",Operator);
}

void cSearch::pushValue(const char* Start, const char* End){
    const char* Value = string(Start,End).c_str();
    if(!Value || !strcmp(Value,"")) return;

    this->CurrentValue = strdup(Value);
    MESSAGE(VERBOSE_PARSERS, "Value %s added", Value);
}

void cSearch::pushExistance(const char* Exists){
    this->CurrentValue = strdup(Exists);
    this->CurrentOperator = strdup("IS");
    MESSAGE(VERBOSE_PARSERS, "Existance expression called. '%s'", Exists);
}

void cSearch::pushConcatOp(const char* Operator){
    if(asprintf(&this->SQLWhereStmt, "%s %s ", this->SQLWhereStmt, Operator)==-1){
        ERROR("Unable to allocate SQL Statement");
        return;
    }

    MESSAGE(VERBOSE_PARSERS, "Concatenation expression called. '%s'", Operator);
}

void cSearch::pushStartBrackedExp(const char){
    MESSAGE(VERBOSE_PARSERS, "Pushing opening bracket");
    if(asprintf(&this->SQLWhereStmt, "%s(", this->SQLWhereStmt)==-1){
        ERROR("Unable to allocate SQL Statement");
        return;
    }
}

 /**********************************************\
 *                                              *
 *  The rest                                    *
 *                                              *
 \**********************************************/

cSearch* cSearch::mInstance = NULL;

const char* cSearch::parse(const char* Search){
    if(!cSearch::mInstance) cSearch::mInstance = new cSearch();

    if(cSearch::mInstance && cSearch::mInstance->parseCriteria(Search)){
        return cSearch::mInstance->SQLWhereStmt;
    }
    return NULL;
}

bool cSearch::parseCriteria(const char* Search){

    charCallback endBrackedExpCB(bind(&cSearch::pushEndBrackedExp, this, _1));
    expCallback pushSimpleExpCB(bind(&cSearch::pushExpression, this, _1, _2));
    opCallback pushOperatorCB(bind(&cSearch::pushOperator, this, _1));
    expCallback pushQuotedValueCB(bind(&cSearch::pushValue, this, _1, _2));
    opCallback pushExistanceCB(bind(&cSearch::pushExistance, this, _1));
    propCallback pushPropertyCB(bind(&cSearch::pushProperty, this, _1));
    opCallback pushConcatOpCB(bind(&cSearch::pushConcatOp, this, _1));
    charCallback startBrackedExpCB(bind(&cSearch::pushStartBrackedExp, this, _1));

    // Craft the grammar
    cSearchGrammar Grammar(endBrackedExpCB,
                           pushSimpleExpCB,
                           pushOperatorCB,
                           pushQuotedValueCB,
                           pushExistanceCB,
                           pushPropertyCB,
                           pushConcatOpCB,
                           startBrackedExpCB);

    MESSAGE(VERBOSE_PARSERS, "Starting search parsing");

    if(boost::spirit::parse(Search, Grammar).full){
        MESSAGE(VERBOSE_DIDL, "Parsed search expression successfuly");
    }
    else {
        ERROR("Parsing search expression failed");
        return false;
    }
    return true;
}

cSearch::cSearch(){
    this->CurrentOperator = NULL;
    this->CurrentProperty = NULL;
    this->CurrentValue    = NULL;
    this->SQLWhereStmt    = strdup("");
}

cSearch::~cSearch(){
    delete this->CurrentOperator;
    delete this->CurrentProperty;
    delete this->CurrentValue;
}

 /**********************************************\
 *                                              *
 *  The filter                                  *
 *                                              *
 \**********************************************/

cFilterCriteria::cFilterCriteria(){
    this->mFilterList = NULL;
}

cFilterCriteria::~cFilterCriteria(){}

cStringList* cFilterCriteria::parse(const char* Filter){
    cFilterCriteria* FilterParser = new cFilterCriteria;
    cStringList* List = NULL;

    if(FilterParser && FilterParser->parseFilter(Filter)){
        List = FilterParser->getFilterList();
    }

    delete FilterParser;

    return List;
}

bool cFilterCriteria::parseFilter(const char* Filter){
    this->mFilterList = new cStringList;

    if (strcmp(Filter, "*") != 0){
		MESSAGE(VERBOSE_CDS, "Got a non wildcard filter %s",Filter);
	}

	//just to test if the filter is the problem
	if (strcmp(Filter, "") == 0) Filter = "*";

    if(Filter && !strcasecmp(Filter, "")){
        MESSAGE(VERBOSE_PARSERS, "Empty filter");
        return true;
    }

    charCallback pushAsteriskCB(bind(&cFilterCriteria::pushAsterisk,this,_1));
    propCallback pushPropertyCB(bind(&cFilterCriteria::pushProperty,this,_1));

    cFilterGrammar Grammar(pushPropertyCB, pushAsteriskCB);

    if(boost::spirit::parse(Filter, Grammar).full){
        MESSAGE(VERBOSE_PARSERS, "Parse filter successful");
    }
    else {
        ERROR("Parsing filter failed");
        return false;
    }
    return true;
}

 /**********************************************\
 *                                              *
 *  The actors                                  *
 *                                              *
 \**********************************************/

void cFilterCriteria::pushProperty(const char* Property){
    MESSAGE(VERBOSE_PARSERS, "Pushing property");
    this->mFilterList->Append(strdup(Property));
}

void cFilterCriteria::pushAsterisk(const char){
    MESSAGE(VERBOSE_PARSERS, "Pushing asterisk (*)");
    if(this->mFilterList) delete this->mFilterList;
    this->mFilterList = NULL;
    return;
}

 /**********************************************\
 *                                              *
 *  The sorter                                  *
 *                                              *
 \**********************************************/

cSortCriteria::cSortCriteria(){
    this->mCriteriaList = new cList<cSortCrit>;
    this->mCurrentCrit = NULL;
}

cSortCriteria::~cSortCriteria(){}

cList<cSortCrit>* cSortCriteria::parse(const char* Sort){
    cSortCriteria* SortParser = new cSortCriteria;
    cList<cSortCrit>* List = NULL;
    if(SortParser && SortParser->parseSort(Sort)){
        List = SortParser->getSortList();
    }

    delete SortParser;

    return List;
}

bool cSortCriteria::parseSort(const char* Sort){
    if(!Sort || !strcasecmp(Sort, "")){
        MESSAGE(VERBOSE_PARSERS, "Empty Sort");
        return true;
    }

    charCallback pushDirectionCB(bind(&cSortCriteria::pushDirection,this,_1));
    propCallback pushPropertyCB(bind(&cSortCriteria::pushProperty,this,_1));

    cSortGrammar Grammar(pushPropertyCB, pushDirectionCB);

    if(boost::spirit::parse(Sort, Grammar).full){
        MESSAGE(VERBOSE_PARSERS, "Parse Sort successful");
    }
    else {
        ERROR("Parsing Sort failed");
        return false;
    }
    return true;
}

 /**********************************************\
 *                                              *
 *  The actors                                  *
 *                                              *
 \**********************************************/

void cSortCriteria::pushProperty(const char* Property){
    MESSAGE(VERBOSE_PARSERS, "Pushing property '%s'", Property);
    this->mCurrentCrit->Property = strdup(Property);
    this->mCriteriaList->Add(this->mCurrentCrit);
    return;
}

void cSortCriteria::pushDirection(const char Direction){
    MESSAGE(VERBOSE_PARSERS, "Pushing direction '%c'", Direction);
    this->mCurrentCrit = new cSortCrit;
    this->mCurrentCrit->SortDescending = (Direction=='-')?true:false;
    return;
}

 /**********************************************\
 *                                              *
 *  The pathparser                              *
 *                                              *
 \**********************************************/

/** @private */
struct cWebserverSections : symbols<> {
    cWebserverSections(){
        add
                (UPNP_DIR_SHARES)
                ;
    }
} WebserverSections;

/** @private */
struct cWebserverMethods : symbols<int> {
    cWebserverMethods(){
        add
                ("browse", UPNP_WEB_METHOD_BROWSE)
                ("download", UPNP_WEB_METHOD_DOWNLOAD)
                ("search", UPNP_WEB_METHOD_SEARCH)
                ("show", UPNP_WEB_METHOD_SHOW)
                ("get", UPNP_WEB_METHOD_STREAM)
                ;
    }
} WebserverMethods;

/** @private */
struct cPathParserGrammar : public boost::spirit::grammar<cPathParserGrammar> {

    intCallback &pushSection;
    intCallback  &pushMethod;
    expCallback  &pushPropertyKey;
    expCallback  &pushPropertyValue;

    cPathParserGrammar(intCallback &pushSection,
                       intCallback  &pushMethod,
                       expCallback  &pushPropertyKey,
                       expCallback  &pushPropertyValue):
                      pushSection(pushSection),
                      pushMethod(pushMethod),
                      pushPropertyKey(pushPropertyKey),
                      pushPropertyValue(pushPropertyValue){}

    template <typename scanner>
    /** @private */
    struct definition {
        boost::spirit::rule<scanner> pathExp, section, method, methodProperties,
                                     property, key, value, uncriticalChar;

        const boost::spirit::rule<scanner> &start(){
            return pathExp;
        }
        definition(const cPathParserGrammar &self){
            pathExp             = section >> ch_p('/') >> method >> ch_p('?') >> methodProperties
                                  ;

            section             = WebserverSections[self.pushSection]
                                  ;

            method              = WebserverMethods[self.pushMethod]
                                  ;

            methodProperties    = property >> *(ch_p('&') >> methodProperties)
                                  ;

            property            = key >> ch_p('=') >> value
                                  ;

            key                 = (+alnum_p)[self.pushPropertyKey]
                                  ;

            value               = (*uncriticalChar)[self.pushPropertyValue]
                                  ;

            uncriticalChar      = chset_p("-_.%~0-9A-Za-z")
                                  ;
        }
    };
};

cPathParser::cPathParser(){
    this->mSection = 0;
    this->mMethod = 0;
}

cPathParser::~cPathParser(){}

bool cPathParser::parse(const char* Path, int* Section, int* Method, propertyMap* Properties){
    cPathParser* Parser = new cPathParser();
    bool ret = (Parser && Parser->parsePath(Path, Section, Method, Properties)) ? true : false;

    delete Parser;

    return ret;
}

bool cPathParser::parsePath(const char* Path, int* Section, int* Method, propertyMap* Properties){
    if(!Path){
        return false;
    }

    intCallback pushSectionCB(bind(&cPathParser::pushSection,this,_1));
    intCallback pushMethodCB(bind(&cPathParser::pushMethod,this,_1));
    expCallback pushPropertyKeyCB(bind(&cPathParser::pushPropertyKey, this, _1, _2));
    expCallback pushPropertyValueCB(bind(&cPathParser::pushPropertyValue, this, _1, _2));

    cPathParserGrammar Grammar(pushSectionCB, pushMethodCB, pushPropertyKeyCB, pushPropertyValueCB);

    if(boost::spirit::parse(Path, Grammar).full){
        MESSAGE(VERBOSE_PARSERS, "Parse path successful");
        *Section = this->mSection;
        *Method = this->mMethod;
        *Properties = this->mProperties;
        return true;
    }
    else {
        ERROR("Parsing path failed");
        return false;
    }

    return true;
}

 /**********************************************\
 *                                              *
 *  The actors                                  *
 *                                              *
 \**********************************************/

void cPathParser::pushPropertyKey(const char* Start, const char* End){
    char* Key = strndup(Start, End-Start);

    MESSAGE(VERBOSE_PARSERS, "Pushing key '%s'", Key);

    this->mKey = Key;

    free(Key);
}

void cPathParser::pushPropertyValue(const char* Start, const char* End){
    char* Value = strndup(Start, End-Start);

    MESSAGE(VERBOSE_PARSERS, "Pushing value '%s'", Value);
    // TODO: urlDecode Value

    if(*this->mKey){
        char* Key = strdup(this->mKey);
        this->mProperties[Key] = Value;
    }
}

void cPathParser::pushMethod(int Method){
    MESSAGE(VERBOSE_PARSERS, "Pushing method '%d'", Method);
    this->mMethod = Method;
}

void cPathParser::pushSection(int Section){
    MESSAGE(VERBOSE_PARSERS, "Pushing section '%d'", Section);
    this->mSection = Section;
}
#ifndef LEARNING_NAIVE_BAYES_CLASSIFIER_H
#define LEARNING_NAIVE_BAYES_CLASSIFIER_H

#include "../globals.h"
#include "classifier.h"

typedef unsigned int COUNTER;

class NBClassifier : public Classifier {
public:
    NBClassifier();
    virtual ~NBClassifier();

    virtual void buildClassifier(int num_classes);
    virtual void addExample(const void *obj, int tag);
    virtual bool distributionForInstance(const void *obj, double *dist);
private:
    double NBconditionalProb(const void *obj, int classVal);

    /**
       * 3D array (m_NumClasses * m_TotalAttValues * m_TotalAttValues)
       * of attribute counts, i.e., the number of times an attribute value occurs
       * in conjunction with another attribute value and a class value.
       */
    //COUNTER ***m_CondiCounts;
    COUNTER **m_CondiCounts1;

    /** The number of times each class value occurs in the dataset */
    COUNTER *m_ClassCounts;

    /** The sums of attribute-class counts
     *    -- if there are no missing values for att, then
     *       m_SumForCounts[classVal][att] will be the same as
     *       m_ClassCounts[classVal]
     */
    COUNTER **m_SumForCounts;

    /** The number of classes */
    int m_NumClasses;

    /** The number of attributes in dataset, including class */
    int m_NumAttributes;

    /** The number of instances in the dataset */
    int m_NumInstances;

    /** The index of the class attribute */
    int m_ClassIndex;

    /**
     * The total number of values (including an extra for each attribute's
     * missing value, which are included in m_CondiCounts) for all attributes
     * (not including class). E.g., for three atts each with two possible values,
     * m_TotalAttValues would be 9 (6 values + 3 missing).
     * This variable is used when allocating space for m_CondiCounts matrix.
     */
    int m_TotalAttValues;

    /** The starting index (in the m_CondiCounts matrix) of the values for each
     * attribute */
    int *m_StartAttIndex;

    /** The number of values for each attribute */
    int *m_NumAttValues;

    /** The frequency of each attribute value for the dataset */
    COUNTER *m_Frequencies;

    /** The number of valid class values observed in dataset
     *  -- with no missing classes, this number is the same as m_NumInstances.
     */
    COUNTER m_SumInstances;

    /** An att's frequency must be this value or more to be a superParent */
    int m_Limit;

    /** If true, outputs debugging info */
    bool m_Debug;

    /** flag for using m-estimates */
    bool m_MEstimates;

    /** value for m in m-estimate */
    int m_Weight;
};

#endif

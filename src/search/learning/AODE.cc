#include "AODE.h"

static void normalize(double *doubles, int length);

AODEClassifier::AODEClassifier() {
    /** An att's frequency must be this value or more to be a superParent */
    m_Limit = 1;
    /** If true, outputs debugging info */
    m_Debug = false;
    /** flag for using m-estimates */
    m_MEstimates = false;
    /** value for m in m-estimate */
    m_Weight = 1;
}

AODEClassifier::~AODEClassifier() {
    // TODO Auto-generated destructor stub
}


/**
* Generates the classifier.
*
* @param instances set of instances serving as training data
* @throws Exception if the classifier has not been generated
* successfully
*/
void AODEClassifier::buildClassifier(int num_classes) {
    // reset variable for this fold
    m_SumInstances = 0;
    m_ClassIndex = feature_extractor->get_num_features();
    m_NumInstances = 0;
    m_NumAttributes = feature_extractor->get_num_features() + 1;
    m_NumClasses = num_classes;

    // allocate space for attribute reference arrays
    m_StartAttIndex = new int[m_NumAttributes];
    m_NumAttValues = new int[m_NumAttributes];

    m_TotalAttValues = 0;
    for (int i = 0; i < m_NumAttributes; ++i) {
        if (i != m_ClassIndex) {
            m_StartAttIndex[i] = m_TotalAttValues;
            m_NumAttValues[i] = feature_extractor->get_feature_domain_size(i);
            m_TotalAttValues += m_NumAttValues[i] + 1;
            // + 1 so room for missing value count
        } else {
            // m_StartAttIndex[i] = -1;  // class isn't included
            m_NumAttValues[i] = m_NumClasses;
        }
    }

    // allocate space for counts and frequencies
    //m_CondiCounts = new double[m_NumClasses][m_TotalAttValues][m_TotalAttValues];
    //m_SumForCounts = new double*[m_NumClasses][m_NumAttributes];
    m_CondiCounts = new double **[m_NumClasses];
    m_SumForCounts = new double *[m_NumClasses];
    m_ClassCounts = new double[m_NumClasses];
    m_Frequencies = new double[m_TotalAttValues];
    for (int i = 0; i < m_NumClasses; ++i) {
        m_CondiCounts[i] = new double *[m_TotalAttValues];
        for (int j = 0; j < m_TotalAttValues; ++j) {
            m_CondiCounts[i][j] = new double[m_TotalAttValues];
            for (int k = 0; k < m_TotalAttValues; ++k) {
                m_CondiCounts[i][j][k] = 0;
            }
        }
        m_SumForCounts[i] = new double[m_NumAttributes];
        for (int j = 0; j < m_NumAttributes; ++j) {
            m_SumForCounts[i][j] = 0;
        }
        m_ClassCounts[i] = 0;
    }
}

void AODEClassifier::addExample(const void *obj, int tag) {
    double *countsPointer;

    int classVal = tag;
    double weight = 1.0;

    m_ClassCounts[classVal] += weight;
    m_SumInstances += weight;

    vector<int> features;
    feature_extractor->extract_features(obj, features);

    // store instance's att val indexes in an array, b/c accessing it
    // in loop(s) is more efficient
    int *attIndex = new int[m_NumAttributes];
    for (int i = 0; i < m_NumAttributes; ++i) {
        if (i == m_ClassIndex)
            attIndex[i] = -1;  // we don't use the class attribute in counts
        else {
            attIndex[i] = m_StartAttIndex[i] + features[i];
        }
    }

    for (int att1 = 0; att1 < m_NumAttributes; ++att1) {
        if (attIndex[att1] == -1)
            continue;  // avoid pointless looping as Att1 is currently the class attribute

        m_Frequencies[attIndex[att1]] += weight;

        // if this is a missing value, we don't want to increase sumforcounts
        m_SumForCounts[classVal][att1] += weight;

        // save time by referencing this now, rather than do it repeatedly in the loop
        countsPointer = m_CondiCounts[classVal][attIndex[att1]];

        for (int att2 = 0; att2 < m_NumAttributes; ++att2) {
            if (attIndex[att2] != -1) {
                countsPointer[attIndex[att2]] += weight;
            }
        }
    }
}


/**
* Calculates the class membership probabilities for the given test
* instance.
*
* @param instance the instance to be classified
* @return predicted class probability distribution
* @throws Exception if there is a problem generating the prediction
*/
bool AODEClassifier::distributionForInstance(const void *obj, double *dist) {
    vector<int> features;
    feature_extractor->extract_features(obj, features);

    // accumulates posterior probabilities for each class
    double *probs = dist;    //new double[m_NumClasses];
    //double *probs = new double[m_NumClasses];

    // index for parent attribute value, and a count of parents used
    int pIndex, parentCount;

    // pointers for efficiency
    // for current class, point to joint frequency for any pair of att values
    double **countsForClass;
    // for current class & parent, point to joint frequency for any att value
    double *countsForClassParent;

    // store instance's att indexes in an int array, so accessing them
    // is more efficient in loop(s).
    int *attIndex = new int[m_NumAttributes];
    for (int att = 0; att < m_NumAttributes; ++att) {
        if (att == m_ClassIndex)
            attIndex[att] = -1;  // can't use class or missing values in calculations
        else
            attIndex[att] = m_StartAttIndex[att] + features[att];
    }

    // calculate probabilities for each possible class value
    for (int class_val = 0; class_val < m_NumClasses; ++class_val) {
        probs[class_val] = 0;
        double spodeP = 0; // P(X,y) for current parent and class
        parentCount = 0;

        countsForClass = m_CondiCounts[class_val];

        // each attribute has a turn of being the parent
        for (int parent = 0; parent < m_NumAttributes; ++parent) {
            if (attIndex[parent] == -1)
                continue;  // skip class attribute or missing value

            // determine correct index for the parent in m_CondiCounts matrix
            pIndex = attIndex[parent];

            // check that the att value has a frequency of m_Limit or greater
            if (m_Frequencies[pIndex] < m_Limit)
                continue;

            countsForClassParent = countsForClass[pIndex];

            // block the parent from being its own child
            attIndex[parent] = -1;

            ++parentCount;

            // joint frequency of class and parent
            double classparentfreq = countsForClassParent[pIndex];

            // find the number of missing values for parent's attribute
            double missing4ParentAtt =
                m_Frequencies[m_StartAttIndex[parent] + m_NumAttValues[parent]];

            // calculate the prior probability -- P(parent & classVal)
            if (!m_MEstimates) {
                spodeP = (classparentfreq + 1.0)
                         / ((m_SumInstances - missing4ParentAtt) + m_NumClasses
                            * m_NumAttValues[parent]);
            } else {
                spodeP = (classparentfreq + ((double)m_Weight
                                             / (double)(m_NumClasses * m_NumAttValues[parent])))
                         / ((m_SumInstances - missing4ParentAtt) + m_Weight);
            }

            // take into account the value of each attribute
            for (int att = 0; att < m_NumAttributes; ++att) {
                if (attIndex[att] == -1)
                    continue;

                double missingForParentandChildAtt =
                    countsForClassParent[m_StartAttIndex[att] + m_NumAttValues[att]];

                if (!m_MEstimates) {
                    spodeP *= (countsForClassParent[attIndex[att]] + 1.0)
                              / ((classparentfreq - missingForParentandChildAtt)
                                 + m_NumAttValues[att]);
                } else {
                    spodeP *= (countsForClassParent[attIndex[att]]
                               + ((double)m_Weight / (double)m_NumAttValues[att]))
                              / ((classparentfreq - missingForParentandChildAtt)
                                 + m_Weight);
                }
            }

            // add this probability to the overall probability
            probs[class_val] += spodeP;

            // unblock the parent
            attIndex[parent] = pIndex;
        }

        // check that at least one att was a parent
        if (parentCount < 1) {
            // do plain naive bayes conditional prob
            probs[class_val] = NBconditionalProb(features, class_val);
        } else {
            // divide by number of parent atts to get the mean
            probs[class_val] /= (double)(parentCount);
        }
    }

    normalize(probs, m_NumClasses);
    return true;
}


/**
* Calculates the probability of the specified class for the given test
* instance, using naive Bayes.
*
* @param instance the instance to be classified
* @param classVal the class for which to calculate the probability
* @return predicted class probability
*/
double AODEClassifier::NBconditionalProb(vector<int> &features, int classVal) {
    double prob;
    double **pointer;

    // calculate the prior probability
    if (!m_MEstimates) {
        prob = (m_ClassCounts[classVal] + 1.0) / (m_SumInstances + m_NumClasses);
    } else {
        prob = (m_ClassCounts[classVal]
                + ((double)m_Weight / (double)m_NumClasses))
               / (m_SumInstances + m_Weight);
    }
    pointer = m_CondiCounts[classVal];

    // consider effect of each att value
    for (int att = 0; att < m_NumAttributes; ++att) {
        if (att == m_ClassIndex)
            continue;

        // determine correct index for att in m_CondiCounts
        int aIndex = m_StartAttIndex[att] + features[att];

        if (!m_MEstimates) {
            prob *= (double)(pointer[aIndex][aIndex] + 1.0)
                    / ((double)m_SumForCounts[classVal][att] + m_NumAttValues[att]);
        } else {
            prob *= (double)(pointer[aIndex][aIndex]
                             + ((double)m_Weight / (double)m_NumAttValues[att]))
                    / (double)(m_SumForCounts[classVal][att] + m_Weight);
        }
    }
    return prob;
}

/**
 * Normalizes the doubles in the array using the given value.
 *
 * @param doubles the array of double
 * @param sum the value by which the doubles are to be normalized
 * @exception IllegalArgumentException if sum is zero or NaN
 */
static void normalize(double *doubles, int length, double sum) {
    if (sum == 0) {
        // Maybe this should just be a return.
        return;
    }
    for (int i = 0; i < length; ++i) {
        doubles[i] /= sum;
    }
}


/**
   * Normalizes the doubles in the array by their sum.
   *
   * @param doubles the array of double
   * @exception IllegalArgumentException if sum is Zero or NaN
   */
static void normalize(double *doubles, int length) {
    double sum = 0;
    for (int i = 0; i < length; ++i) {
        sum += doubles[i];
    }
    normalize(doubles, length, sum);
}

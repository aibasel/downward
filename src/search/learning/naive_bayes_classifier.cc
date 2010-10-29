#include "naive_bayes_classifier.h"
#include <cassert>

static void normalize(double *doubles, int length);

NBClassifier::NBClassifier() {
    /** An att's frequency must be this value or more to be a superParent */
    m_Limit = 1;
    /** If true, outputs debugging info */
    m_Debug = false;
    /** flag for using m-estimates */
    m_MEstimates = true;
    /** value for m in m-estimate */
    m_Weight = 10;
}

NBClassifier::~NBClassifier() {
    // TODO Auto-generated destructor stub
}


/**
* Generates the classifier.
*
* @param instances set of instances serving as training data
* @throws Exception if the classifier has not been generated
* successfully
*/
void NBClassifier::buildClassifier(int num_classes) {
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
    for (int i = 0; i < m_NumAttributes; i++) {
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

    //cout << "NB mem: " << m_NumClasses * m_TotalAttValues * m_TotalAttValues << endl;

    // allocate space for counts and frequencies
    //m_CondiCounts = new double[m_NumClasses][m_TotalAttValues][m_TotalAttValues];
    //m_SumForCounts = new double*[m_NumClasses][m_NumAttributes];
    //m_CondiCounts = new COUNTER**[m_NumClasses];
    m_CondiCounts1 = new COUNTER *[m_NumClasses];
    m_SumForCounts = new COUNTER *[m_NumClasses];
    m_ClassCounts = new COUNTER[m_NumClasses];
    m_Frequencies = new COUNTER[m_TotalAttValues];
    for (int i = 0; i < m_NumClasses; i++) {
        //m_CondiCounts[i] = new COUNTER*[m_TotalAttValues];
        m_CondiCounts1[i] = new COUNTER[m_TotalAttValues];
        for (int j = 0; j < m_TotalAttValues; j++) {
            m_CondiCounts1[i][j] = 0;
            /*
                m_CondiCounts[i][j] = new COUNTER[m_TotalAttValues];
                for (int k = 0; k < m_TotalAttValues; k++) {
                        m_CondiCounts[i][j][k] = 0;
                }
                */
        }
        m_SumForCounts[i] = new COUNTER[m_NumAttributes];
        for (int j = 0; j < m_NumAttributes; j++) {
            m_SumForCounts[i][j] = 0;
        }
        m_ClassCounts[i] = 0;
    }

    //cout << "NB Built" << endl;
}

void NBClassifier::addExample(const void *obj, int tag) {
    //COUNTER *countsPointer;

    int classVal = tag;
    COUNTER weight = 1;

    m_ClassCounts[classVal] += weight;
    m_SumInstances += weight;

    vector<int> features;
    //features.resize(feature_extractor->get_num_features());
    //cout << "------------  " << features.size() << endl;
    feature_extractor->extract_features(obj, features);
    //cout << "++++++++++++  " << features.size() << endl;

    // store instance's att val indexes in an array, b/c accessing it
    // in loop(s) is more efficient
    int *attIndex = new int[m_NumAttributes];
    for (int i = 0; i < m_NumAttributes; i++) {
        if (i == m_ClassIndex)
            attIndex[i] = -1;  // we don't use the class attribute in counts
        else {
            attIndex[i] = m_StartAttIndex[i] + features[i];
            //cout << i << " " << features[i] << " " << (int) s[i % 11] << endl;
        }
    }

    for (int Att1 = 0; Att1 < m_NumAttributes; Att1++) {
        if (attIndex[Att1] == -1)
            continue;  // avoid pointless looping as Att1 is currently the class attribute

        m_Frequencies[attIndex[Att1]] += weight;

        // if this is a missing value, we don't want to increase sumforcounts
        m_SumForCounts[classVal][Att1] += weight;

        // save time by referencing this now, rather than do it repeatedly in the loop
        m_CondiCounts1[classVal][attIndex[Att1]] += weight;
        /*
        countsPointer = m_CondiCounts[classVal][attIndex[Att1]];

        for(int Att2 = 0; Att2 < m_NumAttributes; Att2++) {
           if(attIndex[Att2] != -1) {
              countsPointer[attIndex[Att2]] += weight;
           }
        }
        */
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
bool NBClassifier::distributionForInstance(const void *obj, double *dist) {
    // calculate probabilities for each possible class value

    for (int classVal = 0; classVal < m_NumClasses; classVal++) {
        dist[classVal] = NBconditionalProb(obj, classVal);
    }

    normalize(dist, m_NumClasses);
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
double NBClassifier::NBconditionalProb(const void *obj, int classVal) {
    double prob;
    //COUNTER **pointer;
    vector<int> features;

    //features.resize(feature_extractor->get_num_features());
    feature_extractor->extract_features(obj, features);

    // calculate the prior probability
    if (!m_MEstimates) {
        prob = (m_ClassCounts[classVal] + 1.0) / ((double)m_SumInstances + m_NumClasses);
    } else {
        prob = (m_ClassCounts[classVal]
                + ((double)m_Weight / (double)m_NumClasses))
               / (m_SumInstances + m_Weight);
    }
    /*
    pointer = m_CondiCounts[classVal];

    // consider effect of each att value
    for(int att = 0; att < m_NumAttributes; att++) {
       if(att == m_ClassIndex)
          continue;

       // determine correct index for att in m_CondiCounts
       int aIndex = m_StartAttIndex[att] + features[att];

       if(!m_MEstimates) {
          prob *= (double)(pointer[aIndex][aIndex] + 1.0)
              / ((double)m_SumForCounts[classVal][att] + m_NumAttValues[att]);
       } else {
          prob *= (double)(pointer[aIndex][aIndex]
                    + ((double)m_Weight / (double)m_NumAttValues[att]))
                 / (double)(m_SumForCounts[classVal][att] + m_Weight);
       }
    }
    */

    COUNTER *pointer1;
    pointer1 = m_CondiCounts1[classVal];
    //double prob1 = prob;

    // consider effect of each att value
    for (int att = 0; att < m_NumAttributes; att++) {
        if (att == m_ClassIndex)
            continue;

        // determine correct index for att in m_CondiCounts
        int aIndex = m_StartAttIndex[att] + features[att];

        if (!m_MEstimates) {
            prob *= (double)(pointer1[aIndex] + 1.0)
                    / ((double)m_SumForCounts[classVal][att] + m_NumAttValues[att]);
        } else {
            prob *= (double)(pointer1[aIndex]
                             + ((double)m_Weight / (double)m_NumAttValues[att]))
                    / (double)(m_SumForCounts[classVal][att] + m_Weight);
        }
    }

    //cout << prob1 << " " << prob << endl;
    //assert( ((prob - prob1) < 0.002) && ((prob1 - prob1 < 0.0002)));


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
    for (int i = 0; i < length; i++) {
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
    for (int i = 0; i < length; i++) {
        sum += doubles[i];
    }
    normalize(doubles, length, sum);
}

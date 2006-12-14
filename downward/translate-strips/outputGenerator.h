#ifndef OUTPUT_GENERATOR_H
#define OUTPUT_GENERATOR_H

class Domain;
class SASEncoding;

class OutputGenerator {
  Domain &domain;
  const SASEncoding &code;

  void printTransitions();
public:
  OutputGenerator(Domain &dom, const SASEncoding &code);
};

#endif

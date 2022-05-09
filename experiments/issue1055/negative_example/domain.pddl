(define (domain latent)
 (:requirements :strips :negative-preconditions)
 (:predicates (p0 ?x0) (p1 ?x0))
 (:action a1 
 :parameters (?x0) :precondition
  (and (not (p0 ?x0))) 
  :effect
  (and (p0 ?x0))
  )
)
(define (domain negative-condition-bug)
  (:predicates (at ?u))
  (:action move
	   :parameters (?u ?v)
	   :precondition (and (at ?u) (not (at ?v)))
	   :effect       (and (at ?v) (not (at ?u)))))

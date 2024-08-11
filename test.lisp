
(begin
   ; This is a comment.
   (define plusthree
      (lambda (y z)
         (if (< z 3) (plusthree (+ y 1) (+ z 1)) y)))
   (define plusn
      (lambda (y z q)
         (plusthree y z)))
   (define pi 3.14)
   (define r 10)
   (define c (+ -1 (* pi (* r r))))
   (define d (plusn 8 0 3))
   (begin
      (write d)))


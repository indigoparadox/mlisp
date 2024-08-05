
(begin
   (define plusthree
      (lambda (y z)
         (if (< z 3) (plusthree (+ y 1) (+ z 1)) y)))
   (define pi 3.14)
   (define r 10)
   (define c (* pi (* r r)))
   (define d (plusthree 8 0))
   (begin
      (write d)))


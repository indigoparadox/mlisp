
(begin
   (define fly
      (lambda (x y vx vy)
         (rectf black 0 0 320 200)
         (rect blue x y 10 10)
         (define vx
            (if (> (+ x vx) 310) -10 10))
         (define vy
            (if (> (+ y vy) 190) -10 10))
         (fly (+ x vx) (+ y vy) vx vy)))
   (fly 0 0 10 10))


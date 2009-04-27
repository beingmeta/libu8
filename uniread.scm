(use-module '{texttools reflection})

 ;name
;general category
;canonical combining class
;bidi class
;dcomp type
;numeric type decimal
;numeric type integral
;numeric type any
;bidi mirrored
;unicode1
;isocomment
;simple uppercase
;simple lowercase
;simple titlecase

(define (convert-entry entry info)
  (if (number? info)
      (and (not (equal? entry ""))
	   (string->number entry info))
      (if (procedure? info) (info entry)
	  (if (find #\space entry) entry
	      (if (equal? entry "") #f (string->lisp entry))))))

(define (parse-decomp string)
  (if (equal? string "") #f
    (map (lambda (x)
	   (if (eqv? (elt x 0) #\<) x (string->number x 16)))
	 (segment string " "))))

(define field-info
  (list 16 #f #f #f #f parse-decomp 10 10 10
	#f #f #f 16 16 16))

(define (segline string)
  (let ((s (segment string ";")))
    (if (= (length s) 14)
	(append s '(""))
      s)))
(define (convert-line string)
  (let ((seg (segline string)))
    (->vector (map convert-entry seg field-info))))
(define (read-unicode-data-file file)
  (let* ((items '()) (input (open-input-file file))
	 (line (getline input))
	 (count 0))
    (until (eof-object? line)
      (let ((converted (convert-line line)))
	(when converted
	  (set! items (cons (convert-line line) items))))
      (set! line (getline input))
      (set! count (1+ count))
      ;; (when (zero? (remainder count 1000))
      ;;   (lineout count "\t" line))
      )
    (->vector (reverse items))))

(define (entry-code entry) (elt entry 0))
(define (entry-name entry) (elt entry 1))
(define (entry-class entry) (elt entry 2))
(define (entry-combining entry) (elt entry 3))
(define (entry-bidi entry) (elt entry 4))
(define (entry-decomp entry) (elt entry 5))
(define (entry-decimal-digit entry) (elt entry 6))
(define (entry-integral-digit entry) (elt entry 7))
(define (entry-digit entry) (elt entry 8))
(define (entry-bidi-mirror entry) (elt entry 9))
(define (entry-unicode1 entry) (elt entry 10))
(define (entry-comment entry) (elt entry 11))
(define (entry-uppercase entry) (elt entry 12))
(define (entry-lowercase entry) (elt entry 13))
(define (entry-titlecase entry) (elt entry 14))



;;;; Generating C data

(define cat2code-alist
  '((ll . 1) ;; lowercase letter
    (lu . 2) ;; uppercase letter
    (lt	 . 3) ;; titlecase letter
    (lm . 4) ;; modifier letter
    (lo . 5) ;; other letter
    (mn . 6) ;; mark, nonspacing
    (me . 6) ;; mark, spacing combining
    (mc . 6) ;; mark, enclosing
    (nd . 7) ;; number
    (nl . 8) ;; number letter
    (no . 8) ;; number other
    (pc . 9) ;; punctuation, connector
    (pd . 9) ;; punctuation, dash
    (ps . 10)  ;; punctuation, open
    (pe . 10) ;; punctuation, close
    (pi . 10) ;; punctuation, initial quote
    (pf . 10) ;; punctuation, final quote
    (po . 10) ;; punctuation, other
    (sm . 11) ;; symbol math
    (sc . 11) ;; symbol currency
    (sk . 11) ;; symbol modifier
    (so . 11) ;; symbol other
    (zs . 12) ;; separator, space
    (zl . 12) ;; separator, line
    (zp . 12) ;; separator, paragraph
    (cc . 12) ;; other, control
    (cf . 10) ;; other, format
    (cs . 13) ;; other, surrogate
    (co . 13) ;; other, private
    (cn . 13) ;; other, not assigned
    ))
(define cat2code
  (let ((table (make-hashtable)))
    (doseq (pair cat2code-alist)
      (add! table (car pair) (cdr pair)))
    table))

(define charmap (make-hashtable))
(define udata
  (if (file-exists? (get-component "unicodedata.dtype"))
      (read-dtype-from-file (get-component "unicodedata.dtype"))
    (read-unicode-data-file (get-component "UnicodeData.txt"))))
(doseq (e udata)
  (store! charmap (elt e 0) e))

(define (points-in-range? from to)
  (do ((scan from (1+ scan)))
      ((or (>= scan to) (exists? (get charmap scan)))
       (not (>= scan to)))))

(define (generate-range from to)
  (if (points-in-range? from to)
      (printout
	"{\n  "
	(do ((scan from (+ scan 2)))
	    ((>= scan to))
	  (unless (= scan from)
	    (if (zero? (remainder (+ scan 2) 16)) (printout ",\n  ")
	       (printout ",")))
	  (let ((c1 (try (get cat2code (entry-class (get charmap
							 scan)))
			 0))
		(c2 (try (get cat2code (entry-class (get charmap (1+
								  scan))))
			 0)))
	    (printout "0x" (number->string (+ (* 16 c1) c2) 16))))
	"}")
    (printout "NULL")))

(define (generate-table from to by)
  (printout
    "{"
    (do ((scan from (+ scan by)))
	((>= scan to))
      (unless (= scan from) (printout ",\n  "))
      (generate-range scan (+ scan by)))
    "}"))

(define trouble {})

(define (caseoff x1 x2)
  (let ((diff (- x1 x2)))
    (if (>= diff 32767) 0
	(if (< diff -32767) 0
	    diff))))

(define (get-data e)
  (let ((codepoint (entry-code e))
	(class (entry-class e)))
    (cond ((eq? class 'll)
	   (if (entry-uppercase e)
	       (caseoff (entry-uppercase e) codepoint)
	       0))
	  ((eq? class 'lu)
	   (if (entry-lowercase e)
	       (caseoff (entry-lowercase e) codepoint)
	       0))
	  ((eq? class 'lt)
	   (if (entry-lowercase e)
	       (caseoff (entry-lowercase e) codepoint)
	       0))
	  ((eq? class 'nd)
	   (or (entry-digit e)
	       (entry-integral-weight e)
	       (entry-decimal-weight e)))
	  (else 0))))

(define (generate-chardata var from to)
  (printout
    "static short " var "[]={\n "
    (do ((scan from (1+ scan)))
	((>= scan to))
      (if (> scan from)
	  (if (and (> (- scan from) 0)
		   (zero? (remainder scan 16)))
	      (begin (printout
		       ", /* "
		       "0x" (number->string (- scan 16) 16)
		       " to 0x" (number->string (- scan 1) 16)
		       " */ \n "))
	      (printout ",")))
      (if (exists? (get charmap scan))
	  (printout (get-data (get charmap scan)))
	  (printout 0)))
    "};"))

;;; Char info

(define (print-info e1)
  (let ((c1 (try (get cat2code (entry-class (get charmap e1)))
		 0))
	(c2 (try (get cat2code (entry-class (get charmap (1+ e1))))
		 0)))
    (printout "0x" (number->string (+ (* 16 c1) c2) 16))))

(define (generate-charinfo var from to)
  (printout
    "static unsigned char " var "[]={\n "
    (do ((scan from (+ scan 2)))
	((>= scan to))
      (if (> scan from)
	  (if (and (> (- scan from) 0)
		   (zero? (remainder scan 16)))
	      (begin (printout
		       ", /* "
		       "0x" (number->string (- scan 16) 16)
		       " to 0x" (number->string (- scan 1) 16)
		       " */ \n "))
	      (printout ",")))
      (if (exists? (get charmap scan))
	  (print-info scan)
	(printout 0)))
    "};"))

(define (generate-decompositions var from to)
  (printout
      "static struct U8_DECOMPOSITION " var "[]={\n"
      (dotimes (i (- to from))
	(let* ((codepoint (+ from i))
	       (entry (get charmap codepoint))
	       (decomp (entry-decomp entry)))
	  (when (and (exists? entry) (pair? decomp))
	    (let ((out (open-output-string)))
	      (doseq (elt decomp)
		(when (integer? elt)
		  (write-char (integer->char elt) out)))
	      (let ((aspacket (string->packet (portdata out))))
		(printout
		    "   {" codepoint ",\""
		    (doseq (byte aspacket)
		      (printout "\\" (number->string byte 8)))
		    "\"} "
		    "/* " (entry-name entry) " */\n"))))))
      "};"))

;;;; The main loop

(define (main prefix limit)
  (let ((limit (parse-arg limit)))
    (generate-charinfo (stringout prefix "_charinfo")
		       0 limit)
    (lineout) (lineout)
    (generate-chardata (stringout prefix "_chardata")
		       0 limit)
    (lineout) (lineout)
    (generate-decompositions (stringout prefix "_decompositions")
		       0 limit)))


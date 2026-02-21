MAIN = main
TEMP = *.aux *.log *.out *.toc *.bbl *.blg *.nav *.snm
PARTS=parts

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex $(PARTS)/*.tex auto/*.tex
	pdflatex $<
	-bibtex $(MAIN).aux
	pdflatex $<
	pdflatex $<

	mkdir -p out && \
	for ext in $(TEMP); do \
		files=$$(ls $$ext 2>/dev/null) && mv -f $$files out/ 2>/dev/null || true; \
	done

	mkdir -p out/$(PARTS) && \
	for ext in $(TEMP); do \
		files=$$(ls $(PARTS)/$$ext 2>/dev/null) && mv -f $$files out/$(PARTS) 2>/dev/null || true; \
	done

	gs -sDEVICE=pdfwrite \
    -dCompatibilityLevel=1.4 \
    -dNOPAUSE \
    -dOptimize=true \
    -dQUIET \
    -dBATCH \
    -dRemoveUnusedFonts=true \
    -dRemoveUnusedImages=true \
    -dOptimizeResources=true \
    -dDetectDuplicateImages \
    -dCompressFonts=true \
    -dEmbedAllFonts=true \
    -dSubsetFonts=true \
    -dPreserveAnnots=true \
    -dPreserveMarkedContent=true \
    -dPreserveOverprintSettings=true \
    -dPreserveHalftoneInfo=true \
    -dPreserveOPIComments=true \
    -dPreserveDeviceN=true \
    -dMaxInlineImageSize=0 \
    -sOutputFile="$(MAIN)_compressed.pdf" \
    "$(MAIN).pdf"
	mv $(MAIN)_compressed.pdf $(MAIN).pdf

clean:
	$(RM) $(TEMP)
	cd $(PARTS) && $(RM) $(TEMP)
	$(RM) -r out

.PHONY: all clean

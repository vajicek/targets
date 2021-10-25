

clean:
	jupyter nbconvert --clear-output --inplace *.ipynb

jupyter:
	./runjupyter.sh

annot:
	python3 -m labelme ~/data/targets/points
	# python3 -m labelme ~/data/targets/squares

licannot:
	labelImg ~/data/licenseplate
	# python3 -m labelme ~/data/targets/squares

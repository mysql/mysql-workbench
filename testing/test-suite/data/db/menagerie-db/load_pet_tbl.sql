# empty the table
DELETE FROM pet;

# load new records into it
LOAD DATA LOCAL INFILE 'pet.txt' INTO TABLE pet;

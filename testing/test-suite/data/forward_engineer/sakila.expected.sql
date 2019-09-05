-- MySQL Workbench Forward Engineering

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

-- -----------------------------------------------------
-- Schema mydb
-- -----------------------------------------------------
SHOW WARNINGS;
-- -----------------------------------------------------
-- Schema sakila
-- -----------------------------------------------------
DROP SCHEMA IF EXISTS `sakila` ;

-- -----------------------------------------------------
-- Schema sakila
-- -----------------------------------------------------
CREATE SCHEMA IF NOT EXISTS `sakila` ;
SHOW WARNINGS;
USE `sakila` ;

-- -----------------------------------------------------
-- Table `sakila`.`actor`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`actor` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`actor` (
  `actor_id` SMALLINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `first_name` VARCHAR(45) NOT NULL,
  `last_name` VARCHAR(45) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`actor_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE KEY `idx_actor_last_name` ON `sakila`.`actor` (`last_name` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`country`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`country` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`country` (
  `country_id` SMALLINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `country` VARCHAR(50) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`country_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`city`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`city` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`city` (
  `city_id` SMALLINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `city` VARCHAR(50) NOT NULL,
  `country_id` SMALLINT(0) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`city_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`address`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`address` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`address` (
  `address_id` SMALLINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `address` VARCHAR(50) NOT NULL,
  `address2` VARCHAR(50) NULL DEFAULT NULL,
  `district` VARCHAR(20) NOT NULL,
  `city_id` SMALLINT(0) UNSIGNED NOT NULL,
  `postal_code` VARCHAR(10) NULL DEFAULT NULL,
  `phone` VARCHAR(20) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`address_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`category`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`category` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`category` (
  `category_id` TINYINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(25) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`category_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`staff`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`staff` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`staff` (
  `staff_id` TINYINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `first_name` VARCHAR(45) NOT NULL,
  `last_name` VARCHAR(45) NOT NULL,
  `address_id` SMALLINT(0) UNSIGNED NOT NULL,
  `picture` BLOB(0) NULL DEFAULT NULL,
  `email` VARCHAR(50) NULL DEFAULT NULL,
  `store_id` TINYINT(0) UNSIGNED NOT NULL,
  `active` (0) NOT NULL DEFAULT TRUE,
  `username` VARCHAR(16) NOT NULL,
  `password` VARCHAR(40) BINARY NULL DEFAULT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`staff_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`store`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`store` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`store` (
  `store_id` TINYINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `manager_staff_id` TINYINT(0) UNSIGNED NOT NULL,
  `address_id` SMALLINT(0) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`store_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`customer`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`customer` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`customer` (
  `customer_id` SMALLINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `store_id` TINYINT(0) UNSIGNED NOT NULL,
  `first_name` VARCHAR(45) NOT NULL,
  `last_name` VARCHAR(45) NOT NULL,
  `email` VARCHAR(50) NULL DEFAULT NULL,
  `address_id` SMALLINT(0) UNSIGNED NOT NULL,
  `active` (0) NOT NULL DEFAULT TRUE,
  `create_date` DATETIME NOT NULL,
  `last_update` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`customer_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE KEY `idx_last_name` ON `sakila`.`customer` (`last_name` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`language`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`language` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`language` (
  `language_id` TINYINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` CHAR(20) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`language_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`film`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`film` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`film` (
  `film_id` SMALLINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `title` VARCHAR(255) NOT NULL,
  `description` TEXT(0) NULL DEFAULT NULL,
  `release_year` YEAR NULL DEFAULT NULL,
  `language_id` TINYINT(0) UNSIGNED NOT NULL,
  `original_language_id` TINYINT(0) UNSIGNED NULL DEFAULT NULL,
  `rental_duration` TINYINT(0) UNSIGNED NOT NULL DEFAULT 3,
  `rental_rate` DECIMAL(4,2) NOT NULL DEFAULT 4.99,
  `length` SMALLINT(0) UNSIGNED NULL DEFAULT NULL,
  `replacement_cost` DECIMAL(5,2) NOT NULL DEFAULT 19.99,
  `rating` ENUM('G','PG','PG-13','R','NC-17') NULL DEFAULT 'G',
  `special_features` SET('Trailers','Commentaries','Deleted Scenes','Behind the Scenes') NULL DEFAULT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`film_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE KEY `idx_title` ON `sakila`.`film` (`title` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`film_actor`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`film_actor` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`film_actor` (
  `actor_id` SMALLINT(0) UNSIGNED NOT NULL,
  `film_id` SMALLINT(0) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`actor_id`, `film_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`film_category`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`film_category` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`film_category` (
  `film_id` SMALLINT(0) UNSIGNED NOT NULL,
  `category_id` TINYINT(0) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`film_id`, `category_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`film_text`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`film_text` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`film_text` (
  `film_id` SMALLINT(0) NOT NULL,
  `title` VARCHAR(255) NOT NULL,
  `description` TEXT(0) NULL DEFAULT NULL,
  PRIMARY KEY (`film_id`))
ENGINE = MyISAM
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE FULLTEXT KEY `idx_title_description` ON `sakila`.`film_text` (`title` ASC, `description` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`inventory`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`inventory` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`inventory` (
  `inventory_id` MEDIUMINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `film_id` SMALLINT(0) UNSIGNED NOT NULL,
  `store_id` TINYINT(0) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`inventory_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE KEY `idx_store_id_film_id` ON `sakila`.`inventory` (`store_id` ASC, `film_id` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`rental`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`rental` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`rental` (
  `rental_id` INT(0) NOT NULL AUTO_INCREMENT,
  `rental_date` DATETIME NOT NULL,
  `inventory_id` MEDIUMINT(0) UNSIGNED NOT NULL,
  `customer_id` SMALLINT(0) UNSIGNED NOT NULL,
  `return_date` DATETIME NULL DEFAULT NULL,
  `staff_id` TINYINT(0) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`rental_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE UNIQUE INDEX ON `sakila`.`rental` (`rental_date` ASC, `inventory_id` ASC, `customer_id` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `sakila`.`payment`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`payment` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `sakila`.`payment` (
  `payment_id` SMALLINT(0) UNSIGNED NOT NULL AUTO_INCREMENT,
  `customer_id` SMALLINT(0) UNSIGNED NOT NULL,
  `staff_id` TINYINT(0) UNSIGNED NOT NULL,
  `rental_id` INT(0) NULL DEFAULT NULL,
  `amount` DECIMAL(5,2) NOT NULL,
  `payment_date` DATETIME NOT NULL,
  `last_update` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`payment_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
USE `sakila` ;

-- -----------------------------------------------------
-- Placeholder table for view `sakila`.`customer_list`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `sakila`.`customer_list` (`ID` INT, `name` INT, `address` INT, `zip code` INT, `phone` INT, `city` INT, `country` INT, `notes` INT, `SID` INT);
SHOW WARNINGS;

-- -----------------------------------------------------
-- Placeholder table for view `sakila`.`film_list`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `sakila`.`film_list` (`FID` INT, `title` INT, `description` INT, `category` INT, `price` INT, `length` INT, `rating` INT, `actors` INT);
SHOW WARNINGS;

-- -----------------------------------------------------
-- Placeholder table for view `sakila`.`nicer_but_slower_film_list`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `sakila`.`nicer_but_slower_film_list` (`FID` INT, `title` INT, `description` INT, `category` INT, `price` INT, `length` INT, `rating` INT, `actors` INT);
SHOW WARNINGS;

-- -----------------------------------------------------
-- Placeholder table for view `sakila`.`staff_list`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `sakila`.`staff_list` (`ID` INT, `name` INT, `address` INT, `zip code` INT, `phone` INT, `city` INT, `country` INT, `SID` INT);
SHOW WARNINGS;

-- -----------------------------------------------------
-- Placeholder table for view `sakila`.`sales_by_store`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `sakila`.`sales_by_store` (`store` INT, `manager` INT, `total_sales` INT);
SHOW WARNINGS;

-- -----------------------------------------------------
-- Placeholder table for view `sakila`.`sales_by_film_category`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `sakila`.`sales_by_film_category` (`category` INT, `total_sales` INT);
SHOW WARNINGS;

-- -----------------------------------------------------
-- Placeholder table for view `sakila`.`actor_info`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `sakila`.`actor_info` (`actor_id` INT, `first_name` INT, `last_name` INT, `film_info` INT);
SHOW WARNINGS;

-- -----------------------------------------------------
-- procedure rewards_report
-- -----------------------------------------------------

USE `sakila`;
DROP procedure IF EXISTS `sakila`.`rewards_report`;
SHOW WARNINGS;

DELIMITER $$
USE `sakila`$$


CREATE PROCEDURE `sakila`.`rewards_report` (
    IN min_monthly_purchases TINYINT UNSIGNED
    , IN min_dollar_amount_purchased DECIMAL(10,2) UNSIGNED
    , OUT count_rewardees INT
)
LANGUAGE SQL
NOT DETERMINISTIC 
READS SQL DATA
SQL SECURITY DEFINER
COMMENT 'Provides a customizable report on best customers'
proc: BEGIN
    
    DECLARE last_month_start DATE;
    DECLARE last_month_end DATE;

    /* Some sanity checks... */
    IF min_monthly_purchases = 0 THEN
        SELECT 'Minimum monthly purchases parameter must be > 0';
        LEAVE proc;
    END IF;
    IF min_dollar_amount_purchased = 0.00 THEN
        SELECT 'Minimum monthly dollar amount purchased parameter must be > $0.00';
        LEAVE proc;
    END IF;

    /* Determine start and end time periods */
    SET last_month_start = DATE_SUB(CURRENT_DATE(), INTERVAL 1 MONTH);
    SET last_month_start = STR_TO_DATE(CONCAT(YEAR(last_month_start),'-',MONTH(last_month_start),'-01'),'%Y-%m-%d');
    SET last_month_end = LAST_DAY(last_month_start);

    /* 
        Create a temporary storage area for 
        Customer IDs.  
    */
    CREATE TEMPORARY TABLE tmpCustomer (customer_id SMALLINT UNSIGNED NOT NULL PRIMARY KEY);

    /* 
        Find all customers meeting the 
        monthly purchase requirements
    */
    INSERT INTO tmpCustomer (customer_id)
    SELECT p.customer_id 
    FROM payment AS p
    WHERE DATE(p.payment_date) BETWEEN last_month_start AND last_month_end
    GROUP BY customer_id
    HAVING SUM(p.amount) > min_dollar_amount_purchased
    AND COUNT(customer_id) > min_monthly_purchases;

    /* Populate OUT parameter with count of found customers */
    SELECT COUNT(*) FROM tmpCustomer INTO count_rewardees;

    /* 
        Output ALL customer information of matching rewardees.
        Customize output as needed.
    */
    SELECT c.* 
    FROM tmpCustomer AS t   
    INNER JOIN customer AS c ON t.customer_id = c.customer_id;

    /* Clean up */
    DROP TABLE tmpCustomer;
END$$

DELIMITER ;
SHOW WARNINGS;

-- -----------------------------------------------------
-- function get_customer_balance
-- -----------------------------------------------------

USE `sakila`;
DROP function IF EXISTS `sakila`.`get_customer_balance`;
SHOW WARNINGS;

DELIMITER $$
USE `sakila`$$


CREATE FUNCTION `sakila`.`get_customer_balance`(p_customer_id INT, p_effective_date DATETIME) RETURNS DECIMAL(5,2)
    DETERMINISTIC
    READS SQL DATA
BEGIN

       #OK, WE NEED TO CALCULATE THE CURRENT BALANCE GIVEN A CUSTOMER_ID AND A DATE
       #THAT WE WANT THE BALANCE TO BE EFFECTIVE FOR. THE BALANCE IS:
       #   1) RENTAL FEES FOR ALL PREVIOUS RENTALS
       #   2) ONE DOLLAR FOR EVERY DAY THE PREVIOUS RENTALS ARE OVERDUE
       #   3) IF A FILM IS MORE THAN RENTAL_DURATION * 2 OVERDUE, CHARGE THE REPLACEMENT_COST
       #   4) SUBTRACT ALL PAYMENTS MADE BEFORE THE DATE SPECIFIED

  DECLARE v_rentfees DECIMAL(5,2); #FEES PAID TO RENT THE VIDEOS INITIALLY
  DECLARE v_overfees INTEGER;      #LATE FEES FOR PRIOR RENTALS
  DECLARE v_payments DECIMAL(5,2); #SUM OF PAYMENTS MADE PREVIOUSLY

  SELECT IFNULL(SUM(film.rental_rate),0) INTO v_rentfees
    FROM film, inventory, rental
    WHERE film.film_id = inventory.film_id
      AND inventory.inventory_id = rental.inventory_id
      AND rental.rental_date <= p_effective_date
      AND rental.customer_id = p_customer_id;

  SELECT IFNULL(SUM(IF((TO_DAYS(rental.return_date) - TO_DAYS(rental.rental_date)) > film.rental_duration,
        ((TO_DAYS(rental.return_date) - TO_DAYS(rental.rental_date)) - film.rental_duration),0)),0) INTO v_overfees
    FROM rental, inventory, film
    WHERE film.film_id = inventory.film_id
      AND inventory.inventory_id = rental.inventory_id
      AND rental.rental_date <= p_effective_date
      AND rental.customer_id = p_customer_id;


  SELECT IFNULL(SUM(payment.amount),0) INTO v_payments
    FROM payment

    WHERE payment.payment_date <= p_effective_date
    AND payment.customer_id = p_customer_id;

  RETURN v_rentfees + v_overfees - v_payments;
END$$

DELIMITER ;
SHOW WARNINGS;

-- -----------------------------------------------------
-- procedure film_in_stock
-- -----------------------------------------------------

USE `sakila`;
DROP procedure IF EXISTS `sakila`.`film_in_stock`;
SHOW WARNINGS;

DELIMITER $$
USE `sakila`$$


CREATE PROCEDURE `sakila`.`film_in_stock`(IN p_film_id INT, IN p_store_id INT, OUT p_film_count INT)
READS SQL DATA
BEGIN
     SELECT inventory_id
     FROM inventory
     WHERE film_id = p_film_id
     AND store_id = p_store_id
     AND inventory_in_stock(inventory_id);

     SELECT FOUND_ROWS() INTO p_film_count;
END$$

DELIMITER ;
SHOW WARNINGS;

-- -----------------------------------------------------
-- procedure film_not_in_stock
-- -----------------------------------------------------

USE `sakila`;
DROP procedure IF EXISTS `sakila`.`film_not_in_stock`;
SHOW WARNINGS;

DELIMITER $$
USE `sakila`$$


CREATE PROCEDURE `sakila`.`film_not_in_stock`(IN p_film_id INT, IN p_store_id INT, OUT p_film_count INT)
READS SQL DATA
BEGIN
     SELECT inventory_id
     FROM inventory
     WHERE film_id = p_film_id
     AND store_id = p_store_id
     AND NOT inventory_in_stock(inventory_id);

     SELECT FOUND_ROWS() INTO p_film_count;
END$$

DELIMITER ;
SHOW WARNINGS;

-- -----------------------------------------------------
-- function inventory_held_by_customer
-- -----------------------------------------------------

USE `sakila`;
DROP function IF EXISTS `sakila`.`inventory_held_by_customer`;
SHOW WARNINGS;

DELIMITER $$
USE `sakila`$$


CREATE FUNCTION `sakila`.`inventory_held_by_customer`(p_inventory_id INT) RETURNS INT
READS SQL DATA
BEGIN
  DECLARE v_customer_id INT;
  DECLARE EXIT HANDLER FOR NOT FOUND RETURN NULL;

  SELECT customer_id INTO v_customer_id
  FROM rental
  WHERE return_date IS NULL
  AND inventory_id = p_inventory_id;

  RETURN v_customer_id;
END$$

DELIMITER ;
SHOW WARNINGS;

-- -----------------------------------------------------
-- function inventory_in_stock
-- -----------------------------------------------------

USE `sakila`;
DROP function IF EXISTS `sakila`.`inventory_in_stock`;
SHOW WARNINGS;

DELIMITER $$
USE `sakila`$$


CREATE FUNCTION `sakila`.`inventory_in_stock`(p_inventory_id INT) RETURNS BOOLEAN
READS SQL DATA
BEGIN
    DECLARE v_rentals INT;
    DECLARE v_out     INT;

    #AN ITEM IS IN-STOCK IF THERE ARE EITHER NO ROWS IN THE rental TABLE
    #FOR THE ITEM OR ALL ROWS HAVE return_date POPULATED

    SELECT COUNT(*) INTO v_rentals
    FROM rental
    WHERE inventory_id = p_inventory_id;

    IF v_rentals = 0 THEN
      RETURN TRUE;
    END IF;

    SELECT COUNT(rental_id) INTO v_out
    FROM inventory LEFT JOIN rental USING(inventory_id)
    WHERE inventory.inventory_id = p_inventory_id
    AND rental.return_date IS NULL;

    IF v_out > 0 THEN
      RETURN FALSE;
    ELSE
      RETURN TRUE;
    END IF;
END$$

DELIMITER ;
SHOW WARNINGS;

-- -----------------------------------------------------
-- View `sakila`.`customer_list`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`customer_list`;
SHOW WARNINGS;
DROP VIEW IF EXISTS `sakila`.`customer_list` ;
SHOW WARNINGS;
USE `sakila`;
--
-- View structure for view `customer_list`
--

CREATE  OR REPLACE VIEW customer_list 
AS 
SELECT cu.customer_id AS ID, CONCAT(cu.first_name, _utf8' ', cu.last_name) AS name, a.address AS address, a.postal_code AS `zip code`,
	a.phone AS phone, city.city AS city, country.country AS country, IF(cu.active, _utf8'active',_utf8'') AS notes, cu.store_id AS SID 
FROM customer AS cu JOIN address AS a ON cu.address_id = a.address_id JOIN city ON a.city_id = city.city_id 
	JOIN country ON city.country_id = country.country_id;
SHOW WARNINGS;

-- -----------------------------------------------------
-- View `sakila`.`film_list`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`film_list`;
SHOW WARNINGS;
DROP VIEW IF EXISTS `sakila`.`film_list` ;
SHOW WARNINGS;
USE `sakila`;
--
-- View structure for view `film_list`
--

CREATE  OR REPLACE VIEW film_list 
AS 
SELECT film.film_id AS FID, film.title AS title, film.description AS description, category.name AS category, film.rental_rate AS price,
	film.length AS length, film.rating AS rating, GROUP_CONCAT(CONCAT(actor.first_name, _utf8' ', actor.last_name) SEPARATOR ', ') AS actors 
FROM category LEFT JOIN film_category ON category.category_id = film_category.category_id LEFT JOIN film ON film_category.film_id = film.film_id
        JOIN film_actor ON film.film_id = film_actor.film_id 
	JOIN actor ON film_actor.actor_id = actor.actor_id 
GROUP BY film.film_id;
SHOW WARNINGS;

-- -----------------------------------------------------
-- View `sakila`.`nicer_but_slower_film_list`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`nicer_but_slower_film_list`;
SHOW WARNINGS;
DROP VIEW IF EXISTS `sakila`.`nicer_but_slower_film_list` ;
SHOW WARNINGS;
USE `sakila`;
--
-- View structure for view `nicer_but_slower_film_list`
--

CREATE  OR REPLACE VIEW nicer_but_slower_film_list 
AS 
SELECT film.film_id AS FID, film.title AS title, film.description AS description, category.name AS category, film.rental_rate AS price, 
	film.length AS length, film.rating AS rating, GROUP_CONCAT(CONCAT(CONCAT(UCASE(SUBSTR(actor.first_name,1,1)),
	LCASE(SUBSTR(actor.first_name,2,LENGTH(actor.first_name))),_utf8' ',CONCAT(UCASE(SUBSTR(actor.last_name,1,1)),
	LCASE(SUBSTR(actor.last_name,2,LENGTH(actor.last_name)))))) SEPARATOR ', ') AS actors 
FROM category LEFT JOIN film_category ON category.category_id = film_category.category_id LEFT JOIN film ON film_category.film_id = film.film_id
        JOIN film_actor ON film.film_id = film_actor.film_id
	JOIN actor ON film_actor.actor_id = actor.actor_id 
GROUP BY film.film_id;
SHOW WARNINGS;

-- -----------------------------------------------------
-- View `sakila`.`staff_list`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`staff_list`;
SHOW WARNINGS;
DROP VIEW IF EXISTS `sakila`.`staff_list` ;
SHOW WARNINGS;
USE `sakila`;
--
-- View structure for view `staff_list`
--

CREATE  OR REPLACE VIEW staff_list 
AS 
SELECT s.staff_id AS ID, CONCAT(s.first_name, _utf8' ', s.last_name) AS name, a.address AS address, a.postal_code AS `zip code`, a.phone AS phone,
	city.city AS city, country.country AS country, s.store_id AS SID 
FROM staff AS s JOIN address AS a ON s.address_id = a.address_id JOIN city ON a.city_id = city.city_id 
	JOIN country ON city.country_id = country.country_id;
SHOW WARNINGS;

-- -----------------------------------------------------
-- View `sakila`.`sales_by_store`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`sales_by_store`;
SHOW WARNINGS;
DROP VIEW IF EXISTS `sakila`.`sales_by_store` ;
SHOW WARNINGS;
USE `sakila`;
--
-- View structure for view `sales_by_store`
--

CREATE  OR REPLACE VIEW sales_by_store
AS 
SELECT 
CONCAT(c.city, _utf8',', cy.country) AS store
, CONCAT(m.first_name, _utf8' ', m.last_name) AS manager
, SUM(p.amount) AS total_sales
FROM payment AS p
INNER JOIN rental AS r ON p.rental_id = r.rental_id
INNER JOIN inventory AS i ON r.inventory_id = i.inventory_id
INNER JOIN store AS s ON i.store_id = s.store_id
INNER JOIN address AS a ON s.address_id = a.address_id
INNER JOIN city AS c ON a.city_id = c.city_id
INNER JOIN country AS cy ON c.country_id = cy.country_id
INNER JOIN staff AS m ON s.manager_staff_id = m.staff_id
GROUP BY s.store_id
ORDER BY cy.country, c.city;
SHOW WARNINGS;

-- -----------------------------------------------------
-- View `sakila`.`sales_by_film_category`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`sales_by_film_category`;
SHOW WARNINGS;
DROP VIEW IF EXISTS `sakila`.`sales_by_film_category` ;
SHOW WARNINGS;
USE `sakila`;
--
-- View structure for view `sales_by_film_category`
--
-- Note that total sales will add up to >100% because
-- some titles belong to more than 1 category
--

CREATE  OR REPLACE VIEW sales_by_film_category
AS 
SELECT 
c.name AS category
, SUM(p.amount) AS total_sales
FROM payment AS p
INNER JOIN rental AS r ON p.rental_id = r.rental_id
INNER JOIN inventory AS i ON r.inventory_id = i.inventory_id
INNER JOIN film AS f ON i.film_id = f.film_id
INNER JOIN film_category AS fc ON f.film_id = fc.film_id
INNER JOIN category AS c ON fc.category_id = c.category_id
GROUP BY c.name
ORDER BY total_sales DESC;
SHOW WARNINGS;

-- -----------------------------------------------------
-- View `sakila`.`actor_info`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `sakila`.`actor_info`;
SHOW WARNINGS;
DROP VIEW IF EXISTS `sakila`.`actor_info` ;
SHOW WARNINGS;
USE `sakila`;
--
-- View structure for view `actor_info`
--

CREATE  OR REPLACE DEFINER=CURRENT_USER SQL SECURITY INVOKER VIEW actor_info 
AS
SELECT      
a.actor_id,
a.first_name,
a.last_name,
GROUP_CONCAT(DISTINCT CONCAT(c.name, ': ',
		(SELECT GROUP_CONCAT(f.title ORDER BY f.title SEPARATOR ', ')
                    FROM sakila.film f
                    INNER JOIN sakila.film_category fc
                      ON f.film_id = fc.film_id
                    INNER JOIN sakila.film_actor fa
                      ON f.film_id = fa.film_id
                    WHERE fc.category_id = c.category_id
                    AND fa.actor_id = a.actor_id
                 )
             )
             ORDER BY c.name SEPARATOR '; ')
AS film_info
FROM sakila.actor a
LEFT JOIN sakila.film_actor fa
  ON a.actor_id = fa.actor_id
LEFT JOIN sakila.film_category fc
  ON fa.film_id = fc.film_id
LEFT JOIN sakila.category c
  ON fc.category_id = c.category_id
GROUP BY a.actor_id, a.first_name, a.last_name;
SHOW WARNINGS;

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
USE `sakila`;

DELIMITER $$

USE `sakila`$$
DROP TRIGGER IF EXISTS `sakila`.`ins_film` $$
SHOW WARNINGS$$
USE `sakila`$$

CREATE TRIGGER `ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN
    INSERT INTO film_text (film_id, title, description)
        VALUES (new.film_id, new.title, new.description);
  END$$

SHOW WARNINGS$$

USE `sakila`$$
DROP TRIGGER IF EXISTS `sakila`.`upd_film` $$
SHOW WARNINGS$$
USE `sakila`$$

CREATE TRIGGER `upd_film` AFTER UPDATE ON `film` FOR EACH ROW BEGIN
    IF (old.title != new.title) or (old.description != new.description)
    THEN
        UPDATE film_text
            SET title=new.title,
                description=new.description,
                film_id=new.film_id
        WHERE film_id=old.film_id;
    END IF;
  END$$

SHOW WARNINGS$$

USE `sakila`$$
DROP TRIGGER IF EXISTS `sakila`.`del_film` $$
SHOW WARNINGS$$
USE `sakila`$$

CREATE TRIGGER `del_film` AFTER DELETE ON `film` FOR EACH ROW BEGIN
    DELETE FROM film_text WHERE film_id = old.film_id;
  END$$

SHOW WARNINGS$$

DELIMITER ;

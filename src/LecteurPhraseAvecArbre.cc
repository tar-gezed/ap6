#include "LecteurPhraseAvecArbre.h"

#include <stdlib.h>
#include <iostream>
using namespace std;

// Permet de tester le type d'un opérateur
#define IS_OPBOOL(s)   ((s) == "et" || (s) == "ou")
#define IS_OPREL(s)    ((s) == "==" || (s) == "!=" || (s) == "<" || (s) == ">" || (s) == "<=" || (s) == ">=")
#define IS_OPUNAIRE(s) ((s) == "-"  || (s) == "non")
#define IS_OPADD(s)    ((s) == "+"  || (s) == "-")
#define IS_OPMULT(s)   ((s) == "*"  || (s) == "/")

LecteurPhraseAvecArbre::LecteurPhraseAvecArbre(string nomFich) : ls(nomFich), ts() {

}

void LecteurPhraseAvecArbre::analyse()
{
	arbre = programme();
	cout << "Syntaxe correcte." << endl;
}


// <programme> ::= debut <seqInst> fin FIN_FICHIER
Noeud* LecteurPhraseAvecArbre::programme()
{
	
	sauterSymCour("debut");
	Noeud *si = seqInst();
	sauterSymCour("fin");
	testerSymCour("<FINDEFICHIER>");
	return si;
}


// <seqInst> ::= <inst> ; { <inst> ; }
Noeud* LecteurPhraseAvecArbre::seqInst()
{
	// tant que le symbole courant est un debut possible d'instruction...
	NoeudSeqInst *si = new NoeudSeqInst();
	do {
		si->ajouteInstruction(inst());
		sauterSymCour(";");
	} while (ls.getSymCour() != "fin" &&
		 ls.getSymCour() != "finsi" &&
		 ls.getSymCour() != "sinonsi" &&
		 ls.getSymCour() != "sinon" &&
		 ls.getSymCour() != "fintantque" &&
		 ls.getSymCour() != "jusqua" &&
		 ls.getSymCour() != "finpour");
	return si;
}


// <inst> ::= <affectation> | <instSi> | <instTq> | <instRepeter>
Noeud* LecteurPhraseAvecArbre::inst()
{
	if (ls.getSymCour() == "si")
		return instSi();
	else if (ls.getSymCour() == "tantque")
		return instTq();
	else if (ls.getSymCour() == "repeter")
		return instRepeter();
	else if (ls.getSymCour() == "lire")
		return instLire();
	else if (ls.getSymCour() == "ecrire")
		return instEcrire();
	else if (ls.getSymCour() == "pour")
		return instPour();
	else
		return affectation();
}


//<instSi> ::= si ( <expBool ) <seqInst> { sinonsi ( <expBool> ) <seqInst> } [ sinon <seqInst> ] finsi
Noeud* LecteurPhraseAvecArbre::instSi()
{
	sauterSymCour("si");
	sauterSymCour("(");
	expBool();
	sauterSymCour(")");
	seqInst();

	while (ls.getSymCour() == "sinonsi") {
		sauterSymCour("sinonsi");
		sauterSymCour("(");
		expBool();
		sauterSymCour(")");
		seqInst();
	}

	if (ls.getSymCour() == "sinon") {
		sauterSymCour("sinon");
		seqInst();
	}

	sauterSymCour("finsi");
}


//<instTq> ::= tantque ( <expBool> ) <seqInst> fintantque
Noeud* LecteurPhraseAvecArbre::instTq()
{
	sauterSymCour("tantque");
	sauterSymCour("(");
	expBool();
	sauterSymCour(")");
	seqInst();
	sauterSymCour("fintantque");
}


//<instPour> ::= pour ( <affectation> ; <expBool> ; <affectation> ) <seqInst> finpour
Noeud* LecteurPhraseAvecArbre::instPour()
{
	sauterSymCour("pour");
	sauterSymCour("(");
	affectation();
	sauterSymCour(";");
	expBool();
	sauterSymCour(";");
	affectation();
	sauterSymCour(")");
	seqInst();
	sauterSymCour("finpour");
}


//<instRepeter> ::= repeter <seqInst> jusqua ( <expBool> )
Noeud* LecteurPhraseAvecArbre::instRepeter()
{
	sauterSymCour("repeter");
	seqInst();
	sauterSymCour("jusqua");
	sauterSymCour("(");
	expBool();
	sauterSymCour(")");
}


// <affectation> ::= <variable> = <expression>
Noeud* LecteurPhraseAvecArbre::affectation()
{
	testerSymCour("<VARIABLE>");
	Noeud* var = ts.chercheAjoute(ls.getSymCour());
	ls.suivant();
	sauterSymCour("=");
	Noeud* exp = expression();
	return new NoeudAffectation(var, exp);
}


//<expression> ::= <terme> { <opAdd> <terme> }
Noeud* LecteurPhraseAvecArbre::expression()
{
	Noeud *term = terme();
	while (IS_OPADD(ls.getSymCour())) {
		Symbole op = opAdd();
		term = new NoeudOperateurBinaire(op, term, terme());
	}
	return term;
}


//<terme> ::= <facteur> { <opMult> <facteur> }
Noeud* LecteurPhraseAvecArbre::terme()
{
	Noeud *fact = facteur();
	while (IS_OPMULT(ls.getSymCour())) {
		Symbole op = opMult();
		fact = new NoeudOperateurBinaire(op, fact, facteur());
	}
	return fact;
}


// <facteur> ::= <entier> | <variable> | <opUnaire> <expBool> | ( <expBool> )
Noeud* LecteurPhraseAvecArbre::facteur()
{
	Noeud *fact = NULL;
	
	if (ls.getSymCour() == "<VARIABLE>" || ls.getSymCour() == "<ENTIER>" || ls.getSymCour() == "<CHAINE>") {
		fact = ts.chercheAjoute(ls.getSymCour());
		ls.suivant();
	} else if (IS_OPUNAIRE(ls.getSymCour())) {
		ls.suivant();
		fact = expBool();
	} else if (ls.getSymCour() == "(") {
		ls.suivant();
		fact = expBool();
		sauterSymCour(")");
	} else
		erreur("<facteur>");

	return fact;
}


//<opAdd> ::= + | -
Symbole LecteurPhraseAvecArbre::opAdd()
{
	Symbole op;

	if (IS_OPADD(ls.getSymCour())) {
		op = ls.getSymCour();
		ls.suivant();
	} else
		erreur("<opAdd>");
	return op;
}


//<opMult> ::= * | /
Symbole LecteurPhraseAvecArbre::opMult()
{
	Symbole op;

	if (IS_OPMULT(ls.getSymCour())) {
		op = ls.getSymCour();
		ls.suivant();
	} else
		erreur("<opMult>");
	return op;
}


//<expBool> ::= <expBoolEt> { <opBoolOu> <expBoolEt> }
Noeud* LecteurPhraseAvecArbre::expBool()
{
	expBoolEt();
	while (ls.getSymCour() == "ou") {
		opBoolOu();
		expBoolEt();
	}
}


//<expBoolEt> ::= <relation> { <opBoolEt> <relation> }
Noeud* LecteurPhraseAvecArbre::expBoolEt()
{
	relation();
	while ((ls.getSymCour() == "et")) {
		opBoolEt();
		relation();
	}
}


//<opBool> ::= ou
Symbole LecteurPhraseAvecArbre::opBoolOu()
{
	Symbole op;

	if (ls.getSymCour() == "ou") {
		op = ls.getSymCour();
		ls.suivant();
	} else
		erreur("<opBoolOu>");

	return op;
}

//<opBoolEt> ::= et
Symbole LecteurPhraseAvecArbre::opBoolEt()
{
	Symbole op;

	if (ls.getSymCour() == "et") {
		op = ls.getSymCour();
		ls.suivant();
	} else
		erreur("<opBoolEt>");

	return op;
}


//    <relation> ::= <expression> { <opRel> <expression> }
Noeud* LecteurPhraseAvecArbre::relation()
{
	expression();
	while (IS_OPREL(ls.getSymCour())) {
		opRel();
		expression();
	}
}


//       <opRel> ::= == | != | < | <= | > | >= 
Symbole LecteurPhraseAvecArbre::opRel()
{
	Symbole op;

	if (IS_OPREL(ls.getSymCour())) {
		op = ls.getSymCour();
		ls.suivant();
	} else
		erreur("<opRel>");

	return op;
}


//    <opUnaire> ::= - | non
Symbole LecteurPhraseAvecArbre::opUnaire()
{
	Symbole op;

	if (IS_OPUNAIRE(ls.getSymCour())) {
		op = ls.getSymCour();
		ls.suivant();
	} else
		erreur("<opUnaire>");

	return op;
}


//    <instLire> ::= lire ( <variable> )
Noeud* LecteurPhraseAvecArbre::instLire()
{
	sauterSymCour("lire");
	sauterSymCour("(");
	
	Noeud* var = ts.chercheAjoute(ls.getSymCour());

	sauterSymCour("<VARIABLE>");
	sauterSymCour(")");

	return new NoeudInstLire(var);
}


//  <instEcrire> ::= ecrire ( <expression> | <chaine> )
Noeud* LecteurPhraseAvecArbre::instEcrire()
{
	Noeud *ret = NULL;

	sauterSymCour("ecrire");
	sauterSymCour("(");
	if (ls.getSymCour() == "<CHAINE>") {
		ret = ts.chercheAjoute(ls.getSymCour());
		ls.suivant();
	} else {
		ret = expression();
	}
	sauterSymCour(")");

	return new NoeudInstEcrire(ret);
}

// LecteurPhraseAvecArbre::LecteurPhraseAvecArbre(string nomFich) : ls(nomFich), ts() {}


// void LecteurPhraseAvecArbre::analyse()
// {
// 	arbre = programme();
// 	cout << "Syntaxe correcte." << endl;
// }


// // <programme> ::= debut <seq_inst> fin FIN_FICHIER
// Noeud* LecteurPhraseAvecArbre::programme()
// {
// 	sauterSymCour("debut");
// 	Noeud* si = seqInst();
// 	sauterSymCour("fin");
// 	testerSymCour("<FINDEFICHIER>");
// 	return si;
// }


// // <seqInst> ::= <inst> ; { <inst> ; }
// Noeud* LecteurPhraseAvecArbre::seqInst()
// {
// 	NoeudSeqInst* si = new NoeudSeqInst();

// 	// Boucle tant que le symbole courant est un début possible d'instruction
// 	do {
// 		si->ajouteInstruction(inst());
// 		sauterSymCour(";");
// 	} while (ls.getSymCour() == "<VARIABLE>");

// 	return si;
// }


// // <inst> ::= <affectation> | <inst_condi>
// Noeud* LecteurPhraseAvecArbre::inst()
// {
// 	return affectation();
// }


// // <affectation> ::= <variable> = <expression>
// Noeud* LecteurPhraseAvecArbre::affectation()
// {
// 	testerSymCour("<VARIABLE>");
// 	Noeud* var = ts.chercheAjoute(ls.getSymCour());
// 	ls.suivant();
// 	sauterSymCour("=");
// 	Noeud* exp = expression();
// 	return new NoeudAffectation(var, exp);
// }


// // <expression> ::= <facteur> { <opBinaire> <facteur> }
// Noeud* LecteurPhraseAvecArbre::expression()
// {
// 	Noeud* fact = facteur();

// 	while (ls.getSymCour() == "+" || ls.getSymCour() == "-" ||
// 	       ls.getSymCour() == "*" || ls.getSymCour() == "/") {
// 		Symbole operateur = opBinaire(); // on stocke le symbole de l'opérateur
// 		Noeud* factDroit = facteur(); // lecture de l'operande droit
// 		fact = new NoeudOperateurBinaire(operateur, fact, factDroit); // const. du noeud
// 	}

// 	return fact;
// }


// // <facteur> ::= <entier>  |  <variable>  |  - <facteur>  |  ( <expression> )
// Noeud* LecteurPhraseAvecArbre::facteur()
// {
// 	Noeud* fact = NULL;

// 	if (ls.getSymCour() == "<VARIABLE>" || ls.getSymCour() == "<ENTIER>") {
// 		fact = ts.chercheAjoute(ls.getSymCour());
// 		ls.suivant();
// 	} else if (ls.getSymCour() == "-") {
// 		ls.suivant();
// 		// on représente le moins unaire (- facteur) par une soustractin binaire (0 - facteur)
// 		fact = new NoeudOperateurBinaire(Symbole("-"), ts.chercheAjoute(Symbole("0")), facteur());
// 	} else if (ls.getSymCour() == "(") {
// 		ls.suivant();
// 		fact = expression();
// 		sauterSymCour(")");
// 	} else
// 		erreur("<facteur>");

// 	return fact;
// }


// // <opBinaire> ::= + | - | *  | /
// Symbole LecteurPhraseAvecArbre::opBinaire()
// {
// 	Symbole operateur;

// 	if (ls.getSymCour() == "+" || ls.getSymCour() == "-" || 
// 	    ls.getSymCour() == "*" || ls.getSymCour() == "/" ) {
// 		operateur = ls.getSymCour();
// 		ls.suivant();
// 	} else
// 		erreur("<opBinaire>");

// 	return operateur;
// }


void LecteurPhraseAvecArbre::testerSymCour(string ch)
{
	if (ls.getSymCour() != ch) {
		cout << endl << "-------- Erreur ligne " << ls.getLigne()
		     << " - Colonne " << ls.getColonne() << endl << "   Attendu : "
		     << ch << endl << "   Trouve  : " << ls.getSymCour() << endl
		     << endl;
		exit(0); // plus tard, on levera une exception
	}
}


void LecteurPhraseAvecArbre::sauterSymCour(string ch)
{
	testerSymCour(ch);
	ls.suivant();
}


void LecteurPhraseAvecArbre::erreur(string mess)
{
	cout << endl << "-------- Erreur ligne " << ls.getLigne() << " - Colonne "
	     << ls.getColonne() << endl << "   Attendu : " << mess << endl
	     << "   Trouve  : " << ls.getSymCour() << endl << endl;
	exit(0); // plus tard, on levera une exception
}

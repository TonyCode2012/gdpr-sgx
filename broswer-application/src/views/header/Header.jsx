import React, { Component } from "react";
import { connect } from "react-redux";
import { withRouter } from "react-router-dom";
import { Row, Col } from "reactstrap";
import Link from "../../components/links/Link";
import { PathName } from "../../utils/locations";

const origin = {
  showDropdownItem: false
};

class Header extends Component {
  constructor(props) {
    super(props);
    this.state = { ...origin };

    this.goToHomePage = this.goToHomePage.bind(this);
  }

  componentDidMount() {
    this.props.dispatch({ type: "saga_verify_auth" });
  }

  goToHomePage() {
    this.props.history.push("/");
  }

  render() {
    const subpage = PathName !== "/";

    return (
      <div id="app-header" className={`${subpage && "subpage"}`}>
        <Col xs={12} sm={{ size: 10, offset: 1 }} lg={{ size: 8, offset: 2 }}>
          <Row>
            <Col id="header-name" xs={4}>
              <span>GDPR SGX</span>
            </Col>

            <Col id="header-nav" xs={8}>
              <Link dest="home">Home</Link>
              <Link dest="personal" label="Personal">
                Personal
              </Link>
              <Link dest="business" label="Business">
                Business
              </Link>
            </Col>
          </Row>
        </Col>
      </div>
    );
  }
}

const mapStateToProps = state => state.header;

const mapDispatchToProps = dispatch => ({
  dispatch: action => dispatch(action)
});

export default withRouter(
  connect(
    mapStateToProps,
    mapDispatchToProps
  )(Header)
);
